/*
 * Copyright (C) 2024 Bernd
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
#include "ui_metronome.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::metronome {
void initialize_app(ui::NavigationView& nav) {
    nav.push<MetronomeView>();
}
}  // namespace ui::external_app::metronome

extern "C" {

__attribute__((section(".external_app.app_metronome.application_information"), used)) application_information_t _application_information_metronome = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::metronome::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "Metronome",
    /*.bitmap_data = */ {
        0x00,
        0x00,
        0xC0,
        0x43,
        0x20,
        0x66,
        0xA0,
        0x34,
        0x20,
        0x18,
        0xB0,
        0x0C,
        0x10,
        0x04,
        0x10,
        0x06,
        0x10,
        0x0B,
        0x98,
        0x19,
        0x08,
        0x10,
        0xF8,
        0x1F,
        0xF8,
        0x1F,
        0xF8,
        0x1F,
        0xF0,
        0x0F,
        0x00,
        0x00,

    },
    /*.icon_color = */ ui::Color::cyan().v,
    /*.menu_location = */ app_location_t::UTILITIES,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_none */ {'P', 'A', 'B', 'P'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}