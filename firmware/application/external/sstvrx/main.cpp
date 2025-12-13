/*
 * Copyright (C) 2025 StarVore Labs
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
#include "ui_sstvrx.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::sstvrx {

void initialize_app(NavigationView& nav) {
    nav.push<SstvRxView>();
}

}  // namespace ui::external_app::sstvrx

extern "C" {

// Az alkalmazás információ C-linkage-ként, hogy a firmware hívhassa
__attribute__((section(".external_app.app_sstvrx.application_information"), used))
application_information_t _application_information_sstvrx = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::sstvrx::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "SSTV RX",
    /*.bitmap_data = */ {
        0x00,
        0x00,
        0x00,
        0x00,
        0xFE,
        0x7F,
        0x03,
        0xC0,
        0x53,
        0xD5,
        0xAB,
        0xCA,
        0x53,
        0xD5,
        0xAB,
        0xCA,
        0x53,
        0xD5,
        0xAB,
        0xCA,
        0x53,
        0xD5,
        0x03,
        0xC0,
        0xFF,
        0xFF,
        0xFB,
        0xD7,
        0xFE,
        0x7F,
        0x00,
        0x00,
    },
    /*.icon_color = */ ui::Color::yellow().v,
    /*.menu_location = */ app_location_t::RX,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_none */ {'P', 'S', 'R', 'X'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};

}  // extern "C"