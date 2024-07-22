/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#include "ui_protoview.hpp"

#include "audio.hpp"
#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"
#include "file_path.hpp"

using namespace portapack;
using namespace modems;
using namespace ui;

namespace ui::external_app::protoview {

void ProtoView::focus() {
    field_frequency.focus();
}

ProtoView::ProtoView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&rssi,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_volume,
                  &field_frequency,
                  &console});

    field_frequency.set_step(100);

    baseband::set_subghzd_config(0, receiver_model.sampling_rate());
    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();
    receiver_model.enable();
}

void ProtoView::on_data(const ProtoViewDataMessage* message) {
    // todo draw
    console.clear(true);
    std::string tmp = "";
    for (int i = 0; i <= message->maxptr; i++) {
        if (message->times[i] > 0)
            tmp.append("*");
        else
            tmp.append(".");
    }
    console.write(tmp);
}

ProtoView::~ProtoView() {
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();
}

}  // namespace ui::external_app::protoview
