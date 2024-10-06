/*
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

#include "ui_extsensors.hpp"

#include "rtc_time.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"
#include "i2cdevmanager.hpp"

using namespace portapack;
using namespace ui;

namespace ui::external_app::extsensors {

void ExtSensorsView::focus() {
    console.focus();
}

ExtSensorsView::ExtSensorsView(NavigationView& nav)
    : nav_{nav} {
    add_children({&labels,
                  &text_info,
                  &text_gps,
                  &text_orientation,
                  &text_envl1,
                  &text_envl2,
                  &text_envl3,
                  &console});

    prev_scan_int = i2cdev::I2CDevManager::get_autoscan_interval();
    refreshi2c();
    i2cdev::I2CDevManager::set_autoscan_interval(3);  // scan each 3 sec for new i2c devices
}

void ExtSensorsView::on_new_dev() {
    refreshi2c();
}

void ExtSensorsView::refreshi2c() {
    console.clear(true);
    console.writeln("Found I2C devices:");
    auto addrlist = i2cdev::I2CDevManager::get_gev_list_by_addr();
    for (size_t i = 0; i < addrlist.size(); ++i) {
        console.write("0x");
        console.write(to_string_hex(addrlist[i]));
        console.write(", ");
    }
    if (addrlist.size() == 0) console.writeln("No I2C devs found.");
}

ExtSensorsView::~ExtSensorsView() {
    i2cdev::I2CDevManager::set_autoscan_interval(prev_scan_int);
}

void ExtSensorsView::on_any() {
    if (has_data == false) {
        // update text
        text_info.set("Found ext module.");  // todo do an ext module check for type
    }
    has_data = true;
}

void ExtSensorsView::on_gps(const GPSPosDataMessage* msg) {
    on_any();
    std::string tmp = to_string_decimal(msg->lat, 5);
    tmp += "; ";
    tmp += to_string_decimal(msg->lon, 5);
    text_gps.set(tmp);
}

void ExtSensorsView::on_orientation(const OrientationDataMessage* msg) {
    on_any();
    std::string tmp = to_string_dec_uint(msg->angle);
    tmp += (char)176;  // °
    if (msg->tilt < 400) {
        tmp += "; T: " + to_string_dec_int(msg->tilt);
    }
    text_orientation.set(tmp);
}

void ExtSensorsView::on_environment(const EnvironmentDataMessage* msg) {
    on_any();
    std::string tmp = "T: " + to_string_decimal(msg->temperature, 2);  // temperature
    tmp += (char)176;                                                  // °
    tmp += "C";
    tmp += "; H: " + to_string_decimal(msg->humidity, 1) + "%";  // humidity
    text_envl1.set(tmp);
    tmp = "P: " + to_string_decimal(msg->pressure, 1) + " hPa";  // pressure
    text_envl2.set(tmp);
}

void ExtSensorsView::on_light(const LightDataMessage* msg) {
    on_any();
    std::string tmp = "L: " + to_string_dec_int(msg->light) + " LUX";
    text_envl3.set(tmp);
}

}  // namespace ui::external_app::extsensors