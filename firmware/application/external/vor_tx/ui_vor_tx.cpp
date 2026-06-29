/*
 * Copyright (C) 2026 PortaPack Mayhem
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
 * along with this program; if not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "ui_vor_tx.hpp"

#include "string_format.hpp"

using namespace portapack;
using namespace ui;

namespace ui::external_app::vor_tx {

VorTxView::VorTxView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_vor_tx);

    add_children({&labels,
                  &field_radial,
                  &text_radial_unit,
                  &check_ident,
                  &text_status,
                  &tx_view});

    field_radial.set_value(radial_);
    field_radial.on_change = [this](int32_t v) {
        radial_ = v;
        if (transmitting_) {
            update_config();
        }
    };

    check_ident.set_value(ident_);
    check_ident.on_select = [this](Checkbox&, bool v) {
        ident_ = v;
        if (transmitting_) {
            update_config();
        }
    };

    tx_view.on_edit_frequency = [this, &nav]() {
        auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.target_frequency());
        new_view->on_changed = [this](rf::Frequency f) {
            transmitter_model.set_target_frequency(f);
        };
    };

    tx_view.on_start = [this]() {
        if (tx_acknowledged_) {
            start_tx();
            return;
        }
        nav_.display_modal(
            "TX WARNING",
            "VOR is an aviation nav\nband! TX only into a\ndummy load.",
            YESNO,
            [this](bool choice) {
                if (choice) {
                    tx_acknowledged_ = true;
                    start_tx();
                } else {
                    tx_view.set_transmitting(false);
                }
            });
    };

    tx_view.on_stop = [this]() {
        stop_tx();
    };
}

VorTxView::~VorTxView() {
    transmitter_model.disable();
    baseband::shutdown();
}

void VorTxView::focus() {
    tx_view.focus();
}

void VorTxView::update_config() {
    baseband::set_vor_tx_config(static_cast<uint16_t>(radial_), ident_);
}

void VorTxView::start_tx() {
    transmitter_model.set_sampling_rate(1536000);
    transmitter_model.set_baseband_bandwidth(1750000);
    transmitter_model.enable();
    update_config();
    transmitting_ = true;
    tx_view.set_transmitting(true);
    text_status.set("TX radial " + to_string_dec_uint(radial_));
    set_dirty();
}

void VorTxView::stop_tx() {
    transmitter_model.disable();
    transmitting_ = false;
    tx_view.set_transmitting(false);
    text_status.set("Idle");
    set_dirty();
}

}  // namespace ui::external_app::vor_tx
