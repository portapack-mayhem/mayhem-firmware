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
#include "ui_freq_field.hpp"

using namespace portapack;

namespace ui::external_app::p25_tx {

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
        text_status.set("TX (no baseband)");
        tx_view.set_transmitting(true);
    };
    tx_view.on_stop = [this]() {
        transmitter_model.disable();
        text_status.set("Ready");
        tx_view.set_transmitting(false);
    };

    tx_view.on_edit_frequency = [this, &nav]() {
        auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.target_frequency());
        new_view->on_changed = [this](rf::Frequency f) {
            transmitter_model.set_target_frequency(f);
        };
    };
}

P25TxView::~P25TxView() {
    transmitter_model.disable();
}

void P25TxView::focus() {
    field_nac.focus();
}

}  // namespace ui::external_app::p25_tx
