/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "ui_coasterp.hpp"

#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

namespace ui::external_app::coasterp {

void CoasterPagerView::focus() {
    sym_data.focus();
}

CoasterPagerView::~CoasterPagerView() {
    transmitter_model.disable();
    baseband::shutdown();
}

void CoasterPagerView::generate_frame() {
    constexpr uint8_t frame_bytes = 19;
    uint8_t frame[frame_bytes];

    // Preamble (8 bytes)
    for (uint8_t c = 0; c < 8; c++)
        frame[c] = 0x55;

    // Sync word
    frame[8] = 0x2D;
    frame[9] = 0xD4;

    // Data length
    frame[10] = 8;

    // Data
    auto data_bytes = to_byte_array(sym_data.to_integer());
    memcpy(&frame[11], data_bytes.data(), data_bytes.size());

    // Copy for baseband
    memcpy(shared_memory.bb_data.data, frame, frame_bytes);
}

void CoasterPagerView::start_tx() {
    generate_frame();

    transmitter_model.enable();

    baseband::set_fsk_data(19 * 8, 2280000 / 1000, 5000, 32);
}

void CoasterPagerView::on_tx_progress(const uint32_t /*progress*/, const bool done) {
    if (done) {
        if (tx_mode == SINGLE) {
            transmitter_model.disable();
            tx_mode = IDLE;
            tx_view.set_transmitting(false);
        } else if (tx_mode == SCAN) {
            // Increment address
            uint64_t data = sym_data.to_integer();
            uint16_t address = data & 0xFFFF;

            address++;
            data = (data & 0xFFFFFFFFFFFF0000) + address;
            sym_data.set_value(data);

            start_tx();
        }
    }
}

CoasterPagerView::CoasterPagerView(NavigationView& nav) {
    baseband::run_image(portapack::spi_flash::image_tag_fsktx);

    add_children({&labels,
                  &sym_data,
                  &checkbox_scan,
                  &text_message,
                  &tx_view});

    sym_data.set_value(0x44013B30303034BC);

    checkbox_scan.set_value(false);

    generate_frame();

    tx_view.on_edit_frequency = [this, &nav]() {
        auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.target_frequency());
        new_view->on_changed = [this](rf::Frequency f) {
            transmitter_model.set_target_frequency(f);
        };
    };

    tx_view.on_start = [this]() {
        if (tx_mode == IDLE) {
            if (checkbox_scan.value())
                tx_mode = SCAN;
            else
                tx_mode = SINGLE;
            tx_view.set_transmitting(true);
            start_tx();
        }
    };

    tx_view.on_stop = [this]() {
        tx_view.set_transmitting(false);
        tx_mode = IDLE;
    };
}

} /* namespace ui::external_app::coasterp */
