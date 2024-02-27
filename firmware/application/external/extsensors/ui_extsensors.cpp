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

// Code from https://github.com/Flipper-XFW/Xtreme-Apps/tree/04c3a60093e2c2378e79498b4505aa8072980a42/ble_spam/protocols
// Thanks for the work of the original creators!

#include "ui_extsensors.hpp"

#include "rtc_time.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;
using namespace ui;

namespace ui::external_app::extsensors {

void ExtSensorsView::focus() {
}

ExtSensorsView::ExtSensorsView(NavigationView& nav)
    : nav_{nav} {
    add_children({&labels, &text_info, &text_gps, &text_orientation});
}

ExtSensorsView::~ExtSensorsView() {
}

void ExtSensorsView::on_any() {
    if (has_data == false) {
        // update text
        text_info.set("Found ext module, receiving data.");  // todo do an ext module check
    }
    has_data = true;
}

void ExtSensorsView::on_gps(const GPSPosDataMessage* msg) {
    std::string tmp = to_string_decimal(msg->lat, 5);
    tmp += "; ";
    tmp += to_string_decimal(msg->lon, 5);
    text_gps.set(tmp);
}
void ExtSensorsView::on_orientation(const OrientationDataMessage* msg) {
    std::string tmp = to_string_dec_uint(msg->angle);
    tmp += "°";
    if (msg->tilt < 400) {
        tmp += "; T: " + to_string_dec_int(msg->tilt);
    }
    text_orientation.set(tmp);
}

}  // namespace ui::external_app::extsensors