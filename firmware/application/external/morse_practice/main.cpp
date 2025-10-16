/*
 * Copyright (C) 2025 Pezsma
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
#include "ui_morse_practice.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::morse_practice {

void initialize_app(NavigationView& nav) {
    nav.push<MorsePracticeView>();
}

}  // namespace ui::external_app::morse_practice

extern "C" {

// Az alkalmazás információ C-linkage-ként, hogy a firmware hívhassa
__attribute__((section(".external_app.app_morse_practice.application_information"), used))
application_information_t _application_information_morse_practice = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::morse_practice::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "Morse P",
    /*.bitmap_data = */ {
        0x00,
        0x00,
        0xFE,
        0x7F,
        0xFF,
        0xFF,
        0xBB,
        0xD0,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0x0B,
        0xE1,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xEB,
        0xD0,
        0xFF,
        0xFF,
        0xFE,
        0x7F,
        0x70,
        0x00,
        0x30,
        0x00,
        0x10,
        0x00,
        0x00,
        0x00,
    },
    /*.icon_color = */ ui::Color::yellow().v,
    /*.menu_location = */ app_location_t::GAMES,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_none */ {'P', 'A', 'B', 'P'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};

}  // extern "C"
