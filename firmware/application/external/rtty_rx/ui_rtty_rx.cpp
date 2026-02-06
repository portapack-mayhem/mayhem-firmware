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
        &console,
    });
    field_frequency.set_step(100);
    audio::output::start();
    receiver_model.set_hidden_offset(0);
    receiver_model.set_sampling_rate(3072000);       // set the needed baseband SR.
    receiver_model.set_baseband_bandwidth(1750000);  // set  the front-end RF BW filter.
    receiver_model.enable();
    console.enable_scrolling(false);
    // todo send configure message
    for (int i = 0; i < 90; i++) {
        got_message("RTTY RX #" + to_string_dec_int(i) + "\n");
    }
}

void RttyRxView::got_message(std::string msg) {
    con_buff = con_buff + msg;
    if (con_buff.size() > 600) {
        con_buff.erase(0, con_buff.size() - 600);
    }
    console.clear(true);
    console.write(con_buff);
}

RttyRxView::~RttyRxView() {
    receiver_model.set_hidden_offset(0);
    receiver_model.disable();
    baseband::shutdown();
    audio::output::stop();
}

}  // namespace ui::external_app::rtty_rx