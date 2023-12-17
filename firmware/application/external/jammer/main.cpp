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
#include "ui_jammer.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::jammer {
void initialize_app(ui::NavigationView& nav) {
    nav.push<JammerView>();
}
}  // namespace ui::external_app::jammer

extern "C" {

__attribute__((section(".external_app.app_jammer.application_information"), used)) application_information_t _application_information_jammer = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::jammer::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "Jammer",
    /*.bitmap_data = */ {
        0xE0,
        0x07,
        0xF8,
        0x1F,
        0x1C,
        0x38,
        0x0E,
        0x78,
        0x06,
        0x7C,
        0x03,
        0xCE,
        0x03,
        0xC7,
        0x83,
        0xC3,
        0xC3,
        0xC1,
        0xE3,
        0xC0,
        0x73,
        0xC0,
        0x3E,
        0x60,
        0x1E,
        0x70,
        0x1C,
        0x38,
        0xF8,
        0x1F,
        0xE0,
        0x07,
    },
    /*.icon_color = */ ui::Color::green().v,
    /*.menu_location = */ app_location_t::TX,

    /*.m4_app_tag = portapack::spi_flash::image_tag_jammer */ {'P', 'J', 'A', 'M'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}
