/*
 * Copyright (C) 2026 HTotoo
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or  modify
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

#include "ui_rtty_tx.hpp"

#include "audio.hpp"
#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;
using namespace ui;

namespace ui::external_app::rtty_tx {

void RttyTxView::focus() {
    options_baud.focus();
}

RttyTxView::RttyTxView(NavigationView& nav)
    : nav_{nav} {
    add_children({
        &tx_view,
        &labels,
        &options_baud,
        &options_shift,
        &btn_message,
        &text_message,
    });

    options_baud.on_change = [this](size_t, int32_t value) {
        baud_conf = value;
    };
    options_baud.set_by_value(baud_conf);

    options_shift.on_change = [this](size_t, int32_t value) {
        shift = value;
    };
    options_shift.set_by_value(shift);

    btn_message.on_select = [this](Button&) {
        text_prompt(nav_, message, 64, ENTER_KEYBOARD_MODE_ALPHA, [this](std::string& new_message) {
            message = new_message;
            text_message.set(message);  // maybe replace to console
        });
    };
    text_message.set(message);

    tx_view.on_edit_frequency = [this, &nav]() {
        auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.target_frequency());
        new_view->on_changed = [this](rf::Frequency f) {
            transmitter_model.set_target_frequency(f);
        };
    };

    tx_view.on_start = [this]() {
        if (message.empty()) {
            nav_.display_modal("Error", "Message is empty. Please set a message to transmit.");
            return;
        }
        baseband::run_prepared_image(portapack::memory::map::m4_code.base());
        transmitter_model.set_baseband_bandwidth(3072000);
        transmitter_model.set_channel_bandwidth(6000);
        transmitter_model.enable();
        rtty_message.baud = baud_conf;
        rtty_message.shift = shift;
        baudot_encoder.encode(message, rtty_message.data, &rtty_message.data_len, rtty_message.max_len);
        baseband::set_rtty_config(rtty_message);
        tx_view.set_transmitting(true);
    };

    tx_view.on_stop = [this]() {
        stop();
    };
}

void RttyTxView::on_tx_progress(bool done) {
    if (done) stop();
}

void RttyTxView::stop() {
    tx_view.set_transmitting(false);
    transmitter_model.disable();
    baseband::shutdown();
}

RttyTxView::~RttyTxView() {
    stop();
}

}  // namespace ui::external_app::rtty_tx