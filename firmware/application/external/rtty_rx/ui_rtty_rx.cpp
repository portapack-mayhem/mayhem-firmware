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

#include "ui_rtty_rx.hpp"

#include "audio.hpp"
#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;
using namespace ui;

// todo add timeout, so add newline and time when new data arrives after a time.

namespace ui::external_app::rtty_rx {

void RttyRxView::focus() {
    field_frequency.focus();
}

RttyRxView::RttyRxView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());
    add_children({
        &rssi,
        &field_rf_amp,
        &field_lna,
        &field_vga,
        &field_volume,
        &field_frequency,
        &labels,
        &options_baud,
        &console,
    });
    field_frequency.set_step(100);
    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();

    receiver_model.set_hidden_offset(0);
    receiver_model.set_sampling_rate(3072000);       // set the needed baseband SR.
    receiver_model.set_baseband_bandwidth(1750000);  // set  the front-end RF BW filter.
    receiver_model.enable();
    console.enable_scrolling(false);

    options_baud.on_change = [this](size_t, int32_t value) {
        baseband::set_rtty_config(value, 170);
        baud_conf = value;
    };
    options_baud.set_by_value(baud_conf);
    if (baud_conf == 0) {  // to trigger it when not changed
        baseband::set_rtty_config(0, 170);
    }
}

void RttyRxView::got_message(std::string msg) {
    console.write(msg);
}

RttyRxView::~RttyRxView() {
    receiver_model.set_hidden_offset(0);
    receiver_model.disable();
    baseband::shutdown();
    audio::output::stop();
}

void RttyRxView::on_data(const RTTYDataMessage* message) {
    std::string msg = "";
    for (size_t i = 0; i < message->data_len; i++) {
        const auto c = baudot_decoder.decode(message->data[i]);
        if (c != 0)
            msg += c;
    }
    got_message(msg);
}

}  // namespace ui::external_app::rtty_rx