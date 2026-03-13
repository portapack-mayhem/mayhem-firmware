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

using namespace portapack;

namespace ui::external_app::p25_tx {

// P25 Phase 1 frame sync word: 0x5575F5FF77FF as C4FM dibits (MSB-first, 2 bits each)
static const uint8_t p25_fsw_dibits[24] = {
    1, 1, 1, 1,  // 0x55
    1, 3, 1, 1,  // 0x75
    3, 3, 1, 1,  // 0xF5
    3, 3, 3, 3,  // 0xFF
    1, 3, 1, 3,  // 0x77
    3, 3, 3, 3,  // 0xFF
};

void P25TxView::start_tx() {
    // Fill buffer with repeating P25 FSW as a test pattern
    uint8_t dibits[512];
    for (size_t i = 0; i < sizeof(dibits); i++)
        dibits[i] = p25_fsw_dibits[i % 24];

    baseband::run_prepared_image(portapack::memory::map::m4_code.base());
    transmitter_model.enable();
    tx_view.set_transmitting(true);
    text_status.set("Transmitting...");
    transmitting = true;
    baseband::set_p25tx_data(dibits, sizeof(dibits));
}

void P25TxView::stop_tx() {
    transmitter_model.disable();
    baseband::shutdown();
    tx_view.set_transmitting(false);
    text_status.set("Ready");
    transmitting = false;
}

void P25TxView::on_tx_progress(const uint32_t, const bool done) {
    if (done) {
        stop_tx();
    }
}

P25TxView::P25TxView(NavigationView& nav)
    : nav_{nav} {
    add_children({
        &labels_,
        &field_nac,
        &field_sysid,
        &field_wacn,
        &field_rfssid,
        &field_siteid,
        &field_tg,
        &field_vch,
        &text_status,
        &tx_view,
    });

    field_nac.set_value(0x293);
    field_wacn.set_value(0xBEEF0);
    field_sysid.set_value(0x001);
    field_rfssid.set_value(1);
    field_siteid.set_value(1);
    field_tg.set_value(1);
    field_vch.set_value(1);

    tx_view.on_start = [this]() {
        start_tx();
    };
    tx_view.on_stop = [this]() {
        stop_tx();
    };

    tx_view.on_edit_frequency = [this, &nav]() {
        auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.target_frequency());
        new_view->on_changed = [this](rf::Frequency f) {
            transmitter_model.set_target_frequency(f);
        };
    };
}

P25TxView::~P25TxView() {
    stop_tx();
}

void P25TxView::focus() {
    field_nac.focus();
}

}  // namespace ui::external_app::p25_tx
