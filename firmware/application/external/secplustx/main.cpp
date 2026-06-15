/*
 * Copyright (C) 2026 Synray
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

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_secplustx.hpp"
#include "external_app.hpp"

namespace ui::external_app::ui_secplustx {
void initialize_app(ui::NavigationView& nav) {
    nav.push<SecplusTXView>();
}
}  // namespace ui::external_app::ui_secplustx

extern "C" {

__attribute__((section(".external_app.app_secplustx.application_information"), used)) application_information_t _application_information_secplustx = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::ui_secplustx::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,
    /*.app_name = */ "Security+",
    /*.bitmap_data = */ {
        0b00100000,
        0b00000000,
        0b00100000,
        0b00100000,
        0b00100000,
        0b01110000,
        0b00100000,
        0b00100000,
        0b11100000,
        0b00000111,
        0b11110000,
        0b00001111,
        0b00110000,
        0b00001100,
        0b00110000,
        0b00001100,
        0b11110000,
        0b00001111,
        0b11110000,
        0b00001111,
        0b01110000,
        0b00001101,
        0b10110000,
        0b00001110,
        0b01110000,
        0b00001101,
        0b10110000,
        0b00001110,
        0b11110000,
        0b00001111,
        0b11100000,
        0b00000111,
    },
    /*.icon_color = */ ui::Color::yellow().v,
    /*.menu_location = */ app_location_t::TX,
    /*.desired_menu_position = */ -1,
    /*.m4_app_tag = portapack::spi_flash::image_tag_ook */ {'P', 'O', 'O', 'K'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}
