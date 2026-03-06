/*
 * Copyright (C) 2024 HTotoo
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
#include "ui_same_tx.hpp"
#include "theme.hpp"
#include "baseband_api.hpp"
#include "portapack.hpp"
#include "portapack_shared_memory.hpp"
#include "transmitter_model.hpp"

using namespace portapack;

namespace ui::external_app::same_tx {

static constexpr uint32_t SAME_SAMPLES_PER_BIT = 2949;
static constexpr uint32_t SAME_MARK_FREQ = 2083;
static constexpr uint32_t SAME_SPACE_FREQ = 1563;
static constexpr uint32_t SAME_FM_DEVIATION = 5000;
static constexpr uint8_t SAME_REPEAT = 3;

static constexpr char SAME_ORG_CODES[][4] = {"WXR", "EAS", "CIV", "PEP"};
static constexpr char SAME_EVT_CODES[][4] = {
    "RWT",
    "RMT",
    "NPT",
    "NST",
    "NMT",
    "EAN",
    "EAT",
    "NIC",
    "ADR",
    "AVA",
    "AVW",
    "BZW",
    "CFW",
    "CFS",
    "DSW",
    "EQW",
    "EVI",
    "FFW",
    "FFS",
    "FFH",
    "FRW",
    "HLS",
    "HUW",
    "HUH",
    "SVR",
    "TOR",
};

static uint8_t bitrev8(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

static void fmt2d(char* buf, int v) {
    buf[0] = '0' + v / 10;
    buf[1] = '0' + v % 10;
}
static void fmt3d(char* buf, int v) {
    buf[0] = '0' + v / 100;
    buf[1] = '0' + (v / 10) % 10;
    buf[2] = '0' + v % 10;
}

void SameTxView::update_msg_preview() {
    const char* org = SAME_ORG_CODES[field_org.selected_index()];
    const char* evt = SAME_EVT_CODES[field_event_idx.value()];
    int ss = field_fips_state.value();
    int ccc = field_fips_county.value();
    int dh = field_dur_h.value();
    int dm = field_dur_m.value();
    char buf[48];
    char* p = buf;
    // "WXR-RWT-SS0CCC+HHmm"  (preview, no ZCZC header)
    for (const char* s = org; *s; s++) *p++ = *s;
    *p++ = '-';
    for (const char* s = evt; *s; s++) *p++ = *s;
    *p++ = '-';
    fmt2d(p, ss);
    p += 2;
    *p++ = '0';
    fmt3d(p, ccc);
    p += 3;
    *p++ = '+';
    fmt2d(p, dh);
    p += 2;
    fmt2d(p, dm);
    p += 2;
    *p = '\0';
    text_msg.set(buf);
}

void SameTxView::start_tx() {
    const char* org = SAME_ORG_CODES[field_org.selected_index()];
    const char* evt = SAME_EVT_CODES[field_event_idx.value()];
    int ss = field_fips_state.value();
    int ccc = field_fips_county.value();
    int dh = field_dur_h.value();
    int dm = field_dur_m.value();

    // Build full SAME message: "ZCZC-ORG-EVT-SS0CCC+HHMM-0010000-STATION-"
    char msg[64];
    char* p = msg;
    const char* hdr = "ZCZC-";
    for (const char* s = hdr; *s; s++) *p++ = *s;
    for (const char* s = org; *s; s++) *p++ = *s;
    *p++ = '-';
    for (const char* s = evt; *s; s++) *p++ = *s;
    *p++ = '-';
    fmt2d(p, ss);
    p += 2;
    *p++ = '0';
    fmt3d(p, ccc);
    p += 3;
    *p++ = '+';
    fmt2d(p, dh);
    p += 2;
    fmt2d(p, dm);
    p += 2;
    *p++ = '-';
    // Issue time: fixed 0010000 (placeholder), station: SAME-TX
    const char* tail = "0010000-SAMETX--";
    for (const char* s = tail; *s; s++) *p++ = *s;
    *p = '\0';

    auto* words = reinterpret_cast<uint16_t*>(shared_memory.bb_data.data);
    size_t idx = 0;
    for (uint8_t i = 0; i < 16; i++)
        words[idx++] = bitrev8(0xAB);
    for (const char* c = msg; *c; c++)
        words[idx++] = bitrev8(static_cast<uint8_t>(*c));
    words[idx] = 0;

    tx_active_ = true;
    text_status.set("Transmitting...");
    transmitter_model.enable();
    baseband::set_afsk_data(
        SAME_SAMPLES_PER_BIT,
        SAME_MARK_FREQ,
        SAME_SPACE_FREQ,
        SAME_REPEAT,
        SAME_FM_DEVIATION,
        8);
}

void SameTxView::stop_tx() {
    baseband::kill_afsk();
    transmitter_model.disable();
    tx_active_ = false;
    text_status.set("Ready");
    tx_view.set_transmitting(false);
}

void SameTxView::on_tx_progress(const uint32_t progress, const bool done) {
    if (done) {
        stop_tx();
        text_status.set("Done!");
    } else {
        (void)progress;
        text_status.set("TX...");
    }
}

void SameTxView::focus() {
    field_org.focus();
}

SameTxView::SameTxView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_afsk);

    add_children({
        &labels,
        &field_org,
        &field_event_idx,
        &text_evt,
        &field_fips_state,
        &field_fips_county,
        &field_dur_h,
        &field_dur_m,
        &text_msg,
        &text_status,
        &tx_view,
    });

    // Defaults: Ohio, county 007 (Ashtabula), 30 min
    field_fips_state.set_value(39);
    field_fips_county.set_value(7);
    field_dur_h.set_value(0);
    field_dur_m.set_value(30);

    auto refresh = [this](int32_t) {
        text_evt.set(SAME_EVT_CODES[field_event_idx.value()]);
        update_msg_preview();
    };
    field_event_idx.on_change = refresh;
    field_org.on_change = [this](size_t, int32_t) { update_msg_preview(); };
    field_fips_state.on_change = [this](int32_t) { update_msg_preview(); };
    field_fips_county.on_change = [this](int32_t) { update_msg_preview(); };
    field_dur_h.on_change = [this](int32_t) { update_msg_preview(); };
    field_dur_m.on_change = [this](int32_t) { update_msg_preview(); };

    update_msg_preview();

    tx_view.on_start = [this]() {
        if (!tx_active_) {
            start_tx();
            tx_view.set_transmitting(true);
        }
    };
    tx_view.on_stop = [this]() { stop_tx(); };
}

SameTxView::~SameTxView() {
    transmitter_model.disable();
    baseband::shutdown();
}

}  // namespace ui::external_app::same_tx
