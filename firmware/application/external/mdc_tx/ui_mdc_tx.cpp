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

#include "ui_mdc_tx.hpp"
#include "baseband_api.hpp"
#include "portapack.hpp"
#include "portapack_shared_memory.hpp"
#include "transmitter_model.hpp"
#include "ui/ui_receiver.hpp"

using namespace portapack;

namespace ui::external_app::mdc_tx {

static constexpr uint32_t MDC_SAMPLES_PER_BIT = AFSK_TX_SAMPLERATE / 1200U;
static constexpr uint32_t MDC_MARK_FREQ = 1800U;
static constexpr uint32_t MDC_SPACE_FREQ = 1200U;
static constexpr uint32_t MDC_FM_DEVIATION = 2500U;

static constexpr uint8_t MDC_PREAMBLE[7] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
static constexpr uint8_t MDC_SYNC[5] = {0x07, 0x09, 0x2A, 0x44, 0x6F};
static constexpr int MDC_NUM_PREAMBLES = 3;

static constexpr uint8_t MDC_DP_OP = 0x35;
static constexpr uint8_t MDC_DP_ARG = 0x89;
static constexpr uint8_t MDC_DP_CALL_ARG = 0x0D;

static uint8_t mdc_flip8(uint8_t in) {
    uint8_t out = 0;
    for (int i = 0; i < 8; i++) {
        out = static_cast<uint8_t>((out << 1) | (in & 1));
        in >>= 1;
    }
    return out;
}

static uint16_t mdc_flip16(uint16_t in) {
    uint16_t out = 0;
    for (int i = 0; i < 16; i++) {
        out = static_cast<uint16_t>((out << 1) | (in & 1));
        in >>= 1;
    }
    return out;
}

static uint16_t mdc_crc(const uint8_t* data, int length) {
    uint16_t crc = 0;
    for (int i = 0; i < length; i++) {
        uint16_t c = mdc_flip8(data[i]);
        for (int j = 0x80; j != 0; j >>= 1) {
            uint16_t bit = crc & 0x8000;
            crc = static_cast<uint16_t>(crc << 1);
            if (c & static_cast<uint16_t>(j)) bit ^= 0x8000;
            if (bit) crc ^= 0x1021;
        }
    }
    crc = mdc_flip16(crc);
    crc ^= 0xFFFF;
    return crc;
}

// CRC + convolutional encoding + bit interleaving for a 14-byte codeword.
// Uses lbits_ (class member) so no large allocation hits the stack.
void MdcTxView::pack_codeword(uint8_t cw[14]) {
    uint16_t crc = mdc_crc(cw, 4);
    cw[4] = static_cast<uint8_t>(crc & 0xFF);
    cw[5] = static_cast<uint8_t>((crc >> 8) & 0xFF);
    cw[6] = 0x00;

    // Convolutional encoding — csr[7] is only 7 bytes, fine on stack
    uint8_t csr[7] = {};
    for (int i = 0; i < 7; i++) {
        cw[i + 7] = 0;
        for (int j = 0; j <= 7; j++) {
            for (int k = 6; k > 0; k--) csr[k] = csr[k - 1];
            csr[0] = (cw[i] >> j) & 1;
            int b = csr[0] + csr[2] + csr[5] + csr[6];
            cw[i + 7] |= static_cast<uint8_t>((b & 1) << j);
        }
    }

    // Bit interleaving — uses lbits_ class member (112 bytes off the stack)
    for (int i = 0; i < 112; i++) lbits_[i] = 0;
    int l = 0, m = 0;
    for (int i = 0; i < 14; i++) {
        for (int j = 0; j <= 7; j++) {
            lbits_[l] = (cw[i] >> j) & 1;
            l += 16;
            if (l > 111) {
                m++;
                l = m;
            }
        }
    }
    l = 0;
    for (int i = 0; i < 14; i++) {
        cw[i] = 0;
        for (int j = 7; j >= 0; j--) {
            if (lbits_[l]) cw[i] |= static_cast<uint8_t>(1 << j);
            l++;
        }
    }
}

// Writes preamble + sync + one packed codeword into tx_stream_, returns length.
int MdcTxView::build_single(uint8_t op, uint8_t arg, uint8_t id_hi, uint8_t id_lo) {
    int idx = 0;
    for (int i = 0; i < MDC_NUM_PREAMBLES; i++)
        for (int j = 0; j < 7; j++) tx_stream_[idx++] = MDC_PREAMBLE[j];
    for (int j = 0; j < 5; j++) tx_stream_[idx++] = MDC_SYNC[j];

    uint8_t cw[14] = {};
    cw[0] = op;
    cw[1] = arg;
    cw[2] = id_hi;
    cw[3] = id_lo;
    pack_codeword(cw);
    for (int j = 0; j < 14; j++) tx_stream_[idx++] = cw[j];
    return idx;
}

// Writes preamble + sync + two packed codewords into tx_stream_, returns length.
int MdcTxView::build_double(uint8_t op1, uint8_t arg1, uint8_t id1_hi, uint8_t id1_lo, uint8_t op2, uint8_t arg2, uint8_t id2_hi, uint8_t id2_lo) {
    int idx = 0;
    for (int i = 0; i < MDC_NUM_PREAMBLES; i++)
        for (int j = 0; j < 7; j++) tx_stream_[idx++] = MDC_PREAMBLE[j];
    for (int j = 0; j < 5; j++) tx_stream_[idx++] = MDC_SYNC[j];

    uint8_t cw1[14] = {};
    cw1[0] = op1;
    cw1[1] = arg1;
    cw1[2] = id1_hi;
    cw1[3] = id1_lo;
    pack_codeword(cw1);
    for (int j = 0; j < 14; j++) tx_stream_[idx++] = cw1[j];

    uint8_t cw2[14] = {};
    cw2[0] = op2;
    cw2[1] = arg2;
    cw2[2] = id2_hi;
    cw2[3] = id2_lo;
    pack_codeword(cw2);
    for (int j = 0; j < 14; j++) tx_stream_[idx++] = cw2[j];
    return idx;
}

MdcTxView::MdcTxView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_afsk);

    add_children({
        &labels_,
        &field_op,
        &field_id_hi,
        &field_id_lo,
        &field_arg,
        &field_src_hi,
        &field_src_lo,
        &text_status,
        &tx_view,
    });

    tx_view.on_start = [this]() {
        if (!tx_active_) {
            start_tx();
            tx_view.set_transmitting(true);
        }
    };
    tx_view.on_stop = [this]() { stop_tx(); };

    transmitter_model.set_tx_gain(30);

    tx_view.on_edit_frequency = [this, &nav]() {
        auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.target_frequency());
        new_view->on_changed = [this](rf::Frequency f) {
            transmitter_model.set_target_frequency(f);
        };
    };

    field_op.on_change = [this](size_t, int32_t op) {
        switch (op) {
            case 0xFE:
                field_arg.set_value(0x0C);
                break;  // Unstun
            case 0x2B:
                field_arg.set_value(0x00);
                break;  // Stun
            case 0x63:
                field_arg.set_value(0x85);
                break;  // Radio Check
            case 0x11:
                field_arg.set_value(0x8A);
                break;  // Remote Monitor
            default:
                field_arg.set_value(0x00);
                break;
        }
    };
}

MdcTxView::~MdcTxView() {
    transmitter_model.disable();
    baseband::shutdown();
}

void MdcTxView::focus() {
    field_op.focus();
}

void MdcTxView::start_tx() {
    int32_t raw_op = field_op.selected_index_value();
    uint8_t op = (raw_op == 0xFE) ? 0x2B : static_cast<uint8_t>(raw_op);
    uint8_t arg = static_cast<uint8_t>(field_arg.value());
    uint8_t id_hi = static_cast<uint8_t>(field_id_hi.value());
    uint8_t id_lo = static_cast<uint8_t>(field_id_lo.value());
    uint8_t src_hi = static_cast<uint8_t>(field_src_hi.value());
    uint8_t src_lo = static_cast<uint8_t>(field_src_lo.value());

    int stream_len;
    bool is_double = (op == 0x80 || op == 0x81 || op == 0x82);
    if (is_double) {
        stream_len = build_double(MDC_DP_OP, MDC_DP_ARG, id_hi, id_lo,
                                  op, MDC_DP_CALL_ARG, src_hi, src_lo);
    } else {
        stream_len = build_single(op, arg, id_hi, id_lo);
    }

    // NRZI encode tx_stream_ into shared memory words
    auto* words = reinterpret_cast<uint16_t*>(shared_memory.bb_data.data);
    uint8_t prev_bit = 0;
    for (int i = 0; i < stream_len; i++) {
        uint8_t raw = tx_stream_[i];
        uint8_t nrzi = 0;
        for (int bit = 7; bit >= 0; bit--) {
            uint8_t rb = (raw >> bit) & 1;
            uint8_t nb = rb ^ prev_bit;
            prev_bit = rb;
            nrzi = static_cast<uint8_t>((nrzi << 1) | nb);
        }
        words[i] = nrzi;
    }
    words[stream_len] = 0;

    tx_active_ = true;
    text_status.set("TX...");
    transmitter_model.enable();
    baseband::set_afsk_data(
        MDC_SAMPLES_PER_BIT,
        MDC_MARK_FREQ,
        MDC_SPACE_FREQ,
        1,
        MDC_FM_DEVIATION,
        8);
}

void MdcTxView::stop_tx() {
    baseband::kill_afsk();
    transmitter_model.disable();
    tx_active_ = false;
    text_status.set("Ready");
    tx_view.set_transmitting(false);
}

void MdcTxView::on_tx_progress(uint32_t, bool done) {
    if (done) {
        stop_tx();
        text_status.set("Done!");
    }
}

}  // namespace ui::external_app::mdc_tx
