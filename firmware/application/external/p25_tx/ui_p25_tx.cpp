/*
 * Copyright (C) 2025 Sarah Rose
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "ui_p25_tx.hpp"
#include "portapack.hpp"
#include "transmitter_model.hpp"
#include "baseband_api.hpp"
#include "ui_freq_field.hpp"
#include <cstring>

using namespace portapack;

namespace ui::external_app::p25_tx {

static constexpr uint8_t DUID_TSBK = 0x07;
static constexpr uint8_t SS_TSCC = 0x02;

// BCH(63,16) generator matrix from TIA-102.BAAA-A (ccemu/p25/bch.go)
static const uint64_t bch_matrix[16] = {
    0x8000cd930bdd3b2aULL,
    0x4000ab5a8e33a6beULL,
    0x2000983e4cc4e874ULL,
    0x10004c1f2662743aULL,
    0x0800eb9c98ec0136ULL,
    0x0400b85d47ab3bb0ULL,
    0x02005c2ea3d59dd8ULL,
    0x01002e1751eaceecULL,
    0x0080170ba8f56776ULL,
    0x0040c616dfa78890ULL,
    0x0020630b6fd3c448ULL,
    0x00103185b7e9e224ULL,
    0x000818c2dbf4f112ULL,
    0x0004c1f2662743a2ULL,
    0x0002ad6a38ce9afbULL,
    0x00019b2617ba7657ULL,
};

static uint64_t bch_encode(uint16_t data) {
    uint64_t cw = 0;
    for (int i = 0; i < 16; i++)
        if (data & (0x8000u >> i))
            cw ^= bch_matrix[i];
    return cw;
}

// CRC-CCITT bit-by-bit, init=0, final XOR=0xFFFF (ccemu/p25/crc.go)
static uint16_t crc_ccitt_80(uint16_t high, uint64_t low) {
    uint32_t crc = 0;
    for (int i = 15; i >= 0; i--) {
        crc <<= 1;
        if (((crc >> 16) ^ ((high >> i) & 1)) & 1) crc ^= 0x1021;
    }
    for (int i = 63; i >= 0; i--) {
        crc <<= 1;
        if (((crc >> 16) ^ ((uint32_t)((low >> i) & 1))) & 1) crc ^= 0x1021;
    }
    return (uint16_t)((crc & 0xFFFF) ^ 0xFFFF);
}

// 1/2-rate trellis: [state][inputDibit]={outDibit0,outDibit1} (ccemu/p25/trellis.go)
static const uint8_t trellis_table[4][4][2] = {
    {{0, 2}, {3, 0}, {0, 1}, {3, 3}},
    {{3, 2}, {0, 0}, {3, 1}, {0, 3}},
    {{2, 1}, {1, 3}, {2, 2}, {1, 0}},
    {{1, 1}, {2, 3}, {1, 2}, {2, 0}},
};

static void trellis_encode(const uint8_t* in48, uint8_t* out98) {
    uint8_t state = 0;
    for (int i = 0; i < 49; i++) {
        uint8_t d = (i < 48) ? in48[i] : 0;
        out98[i * 2 + 0] = trellis_table[state][d][0];
        out98[i * 2 + 1] = trellis_table[state][d][1];
        state = d;
    }
}

static void data_interleave(const uint8_t* in98, uint8_t* out98) {
    int idx = 0;
    for (int j = 0; j < 97; j += 8) {
        out98[idx++] = in98[j];
        out98[idx++] = in98[j + 1];
    }
    for (int i = 2; i < 7; i += 2)
        for (int j = 0; j < 89; j += 8) {
            out98[idx++] = in98[i + j];
            out98[idx++] = in98[i + j + 1];
        }
}

static int insert_status(const uint8_t* data, int dlen, uint8_t* out, uint8_t ss) {
    int num_ss = (dlen + 34) / 35, remaining = num_ss, oi = 0, i = 1;
    for (int d = 0; d < dlen; d++) {
        out[oi++] = data[d];
        if ((i % 35 == 0) && remaining > 0) {
            out[oi++] = ss;
            remaining--;
        }
        i++;
    }
    while (remaining > 0) {
        out[oi++] = 0;
        if (i % 35 == 0) {
            out[oi++] = ss;
            remaining--;
        }
        i++;
    }
    return oi;
}

static void build_tsbk(uint8_t* out12, uint8_t opcode, uint64_t args) {
    uint16_t high = (1u << 15) | ((uint16_t)opcode << 8);
    uint16_t crc = crc_ccitt_80(high, args);
    out12[0] = (uint8_t)(high >> 8);
    out12[1] = (uint8_t)high;
    for (int i = 0; i < 8; i++) out12[2 + i] = (uint8_t)(args >> (56 - 8 * i));
    out12[10] = (uint8_t)(crc >> 8);
    out12[11] = (uint8_t)crc;
}

static void tsbk_iden_up(uint8_t* out12, uint64_t freq_hz) {
    uint64_t args = ((uint64_t)100 << 51) | ((uint64_t)100 << 32) | ((freq_hz / 5) & 0xFFFFFFFF);
    build_tsbk(out12, 0x3D, args);
}
static void tsbk_net_status(uint8_t* out12, uint32_t wacn, uint16_t sysid) {
    build_tsbk(out12, 0x3B, ((uint64_t)(wacn & 0xFFFFF) << 36) | ((uint64_t)(sysid & 0xFFF) << 24));
}

static void tsbk_grp_v_grant(uint8_t* out12, uint8_t chan_id, uint16_t chan_num, uint16_t tg) {
    uint64_t ch = ((uint64_t)(chan_id & 0xF) << 12) | (chan_num & 0xFFF);
    // opts: bit1=group
    uint64_t args = ((uint64_t)0x02 << 56) | (ch << 40) | ((uint64_t)tg << 24);
    build_tsbk(out12, 0x00, args);
}
static void tsbk_rfss_status(uint8_t* out12, uint16_t sysid, uint8_t rfssid, uint8_t siteid) {
    build_tsbk(out12, 0x3A, ((uint64_t)(sysid & 0xFFF) << 40) | ((uint64_t)rfssid << 32) | ((uint64_t)siteid << 24));
}

static int build_frame(uint8_t* out, uint16_t nac, const uint8_t* tsbk12) {
    uint8_t pre[160];
    int idx = 0;
    static const uint64_t FSW = 0x5575F5FF77FFULL;
    for (int i = 0; i < 24; i++) pre[idx++] = (uint8_t)((FSW >> (46 - i * 2)) & 3);
    uint64_t nid = bch_encode((uint16_t)((nac << 4) | DUID_TSBK));
    for (int i = 0; i < 32; i++) pre[idx++] = (uint8_t)((nid >> (62 - i * 2)) & 3);
    uint8_t in_d[48];
    for (int i = 0; i < 12; i++) {
        in_d[i * 4 + 0] = (tsbk12[i] >> 6) & 3;
        in_d[i * 4 + 1] = (tsbk12[i] >> 4) & 3;
        in_d[i * 4 + 2] = (tsbk12[i] >> 2) & 3;
        in_d[i * 4 + 3] = (tsbk12[i] >> 0) & 3;
    }
    uint8_t enc[98], ilv[98];
    trellis_encode(in_d, enc);
    data_interleave(enc, ilv);
    memcpy(&pre[idx], ilv, 98);
    idx += 98;
    return insert_status(pre, idx, out, SS_TSCC);
}

static constexpr int TX_BUF_SIZE = 512;

static int fill_tx_buffer(uint8_t* buf, uint16_t nac, uint32_t wacn, uint16_t sysid, uint8_t rfssid, uint8_t siteid, uint64_t freq_hz, uint16_t tg, uint16_t vch, int& tsbk_idx) {
    memset(buf, 0, TX_BUF_SIZE);
    int total = 0;
    uint8_t tsbk[12];
    while (true) {
        switch (tsbk_idx % 5) {
            case 0:
                tsbk_iden_up(tsbk, freq_hz);
                break;
            case 1:
                tsbk_net_status(tsbk, wacn, sysid);
                break;
            case 2:
                tsbk_rfss_status(tsbk, sysid, rfssid, siteid);
                break;
            case 3:
                tsbk_grp_v_grant(tsbk, 0, vch, tg);
                break;
            default:
                tsbk_net_status(tsbk, wacn, sysid);
                break;
        }
        tsbk_idx++;
        uint8_t probe[200];
        int flen = build_frame(probe, nac, tsbk);
        if (total + flen > TX_BUF_SIZE) break;
        memcpy(&buf[total], probe, flen);
        total += flen;
    }
    return total;
}

void P25TxView::start_tx() {
    uint8_t dibits[TX_BUF_SIZE];
    int len = fill_tx_buffer(dibits,
                             (uint16_t)field_nac.to_integer(), (uint32_t)field_wacn.to_integer(),
                             (uint16_t)field_sysid.to_integer(), (uint8_t)field_rfssid.to_integer(),
                             (uint8_t)field_siteid.to_integer(), (uint64_t)transmitter_model.target_frequency(),
                             (uint16_t)field_tg.to_integer(), (uint16_t)field_vch.to_integer(),
                             tsbk_idx_);
    transmitter_model.enable();
    tx_view.set_transmitting(true);
    text_status.set("TX TSCC");
    transmitting = true;
    baseband::set_p25tx_data(dibits, (uint16_t)len);
}

void P25TxView::stop_tx() {
    transmitting = false;
    transmitter_model.disable();
    tx_view.set_transmitting(false);
    text_status.set("Ready");
    tsbk_idx_ = 0;
}

void P25TxView::on_tx_progress(const uint32_t, const bool done) {
    if (done && transmitting) {
        uint8_t dibits[TX_BUF_SIZE];
        int len = fill_tx_buffer(dibits,
                                 (uint16_t)field_nac.to_integer(), (uint32_t)field_wacn.to_integer(),
                                 (uint16_t)field_sysid.to_integer(), (uint8_t)field_rfssid.to_integer(),
                                 (uint8_t)field_siteid.to_integer(), (uint64_t)transmitter_model.target_frequency(),
                                 (uint16_t)field_tg.to_integer(), (uint16_t)field_vch.to_integer(),
                                 tsbk_idx_);
        baseband::set_p25tx_data(dibits, (uint16_t)len);
    }
}

P25TxView::P25TxView(NavigationView& nav)
    : nav_{nav} {
    add_children({&labels_, &field_nac, &field_sysid, &field_wacn,
                  &field_rfssid, &field_siteid, &field_tg, &field_vch,
                  &text_status, &tx_view});
    field_nac.set_value(0x293);
    field_wacn.set_value(0xBEEF0);
    field_sysid.set_value(0x001);
    field_rfssid.set_value(1);
    field_siteid.set_value(1);
    field_tg.set_value(1);
    field_vch.set_value(1);
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());
    tx_view.on_start = [this]() { start_tx(); };
    tx_view.on_stop = [this]() { stop_tx(); };
    tx_view.on_edit_frequency = [this, &nav]() {
        auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.target_frequency());
        new_view->on_changed = [this](rf::Frequency f) { transmitter_model.set_target_frequency(f); };
    };
}

P25TxView::~P25TxView() {
    if (transmitting)
        stop_tx();
    transmitter_model.disable();
    baseband::shutdown();
}

void P25TxView::focus() {
    field_nac.focus();
}

}  // namespace ui::external_app::p25_tx
