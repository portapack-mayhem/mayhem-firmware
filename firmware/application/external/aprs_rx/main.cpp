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
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "ui_aprs_rx.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::aprs_rx {

void initialize_app(ui::NavigationView& nav) {
    nav.push<APRSRXView>();
}

}  // namespace ui::external_app::aprs_rx

extern "C" {

__attribute__((section(".external_app.app_aprs_rx.application_information"), used)) application_information_t _application_information_aprs_rx = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::aprs_rx::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "APRS",
    /*.bitmap_data = */ {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0xF0,
        0x0F,
        0x4C,
        0x32,
        0xFE,
        0x7F,
        0x25,
        0xA4,
        0x25,
        0xA4,
        0xFF,
        0xFF,
        0x25,
        0xA4,
        0x25,
        0xA4,
        0xFE,
        0x7F,
        0x4C,
        0x32,
        0xF0,
        0x0F,
        0x00,
        0x00,
        0x00,
        0x00,
    },
    /*.icon_color = */ ui::Color::green().v,
    /*.menu_location = */ app_location_t::RX,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_aprs_rx */ {'P', 'A', 'P', 'R'},
    /*.m4_app_offset = */ 0x00000000,
};

}  // extern "C"
