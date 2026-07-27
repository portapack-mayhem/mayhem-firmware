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

#include "ais_app.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::ais_rx {

void initialize_app(ui::NavigationView& nav) {
    nav.push<AISAppView>();
}

}  // namespace ui::external_app::ais_rx

extern "C" {

__attribute__((section(".external_app.app_ais_rx.application_information"), used)) application_information_t _application_information_ais_rx = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::ais_rx::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "AIS Boats",
    /*.bitmap_data = */ {
        0x00, 0x01, 0x80, 0x01, 0xC0, 0x01, 0xC0, 0x0D,
        0xE0, 0x3D, 0xF0, 0x3D, 0xF8, 0x7D, 0xFC, 0x7D,
        0xFC, 0x7D, 0xFE, 0x7D, 0xFF, 0x7D, 0x00, 0x00,
        0xF8, 0x7F, 0xF8, 0x3F, 0xF0, 0x0F, 0x00, 0x00,
    },
    /*.icon_color = */ ui::Color::green().v,
    /*.menu_location = */ app_location_t::RX,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_ais */ {'P', 'A', 'I', 'S'},
    /*.m4_app_offset = */ 0x00000000,
};

}  // extern "C"
