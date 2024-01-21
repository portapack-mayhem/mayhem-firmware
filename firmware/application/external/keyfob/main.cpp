/*
 * Copyright (C) 2023 Bernd Herzog
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
#include "ui_keyfob.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::keyfob {
void initialize_app(ui::NavigationView& nav) {
    nav.push<KeyfobView>();
}
}  // namespace ui::external_app::keyfob

extern "C" {

__attribute__((section(".external_app.app_keyfob.application_information"), used)) application_information_t _application_information_keyfob = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::keyfob::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "Keyfob",
    /*.bitmap_data = */ {
        0x30,
        0x00,
        0x30,
        0x00,
        0x30,
        0x00,
        0x30,
        0x00,
        0x30,
        0x00,
        0x30,
        0x00,
        0xFC,
        0x00,
        0xCE,
        0x01,
        0x86,
        0x01,
        0xFE,
        0x01,
        0x86,
        0x31,
        0x86,
        0x49,
        0xCE,
        0x87,
        0xFC,
        0x84,
        0xFC,
        0x4B,
        0x78,
        0x30,
    },
    /*.icon_color = */ ui::Color::orange().v,
    /*.menu_location = */ app_location_t::TX,

    /*.m4_app_tag = portapack::spi_flash::image_tag_keyfob */ {'P', 'O', 'O', 'K'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}
