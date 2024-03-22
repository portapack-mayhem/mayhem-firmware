/*
 * Copyright (C) 2024 Mark Thompson
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
#include "ui_audio_test.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::audio_test {
void initialize_app(ui::NavigationView& nav) {
    nav.push<AudioTestView>();
}
}  // namespace ui::external_app::audio_test

extern "C" {

__attribute__((section(".external_app.app_audio_test.application_information"), used)) application_information_t _application_information_audio_test = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::audio_test::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "Audio Test",
    /*.bitmap_data = */ {
        0x00,
        0x00,
        0x40,
        0x10,
        0x60,
        0x20,
        0x70,
        0x44,
        0x78,
        0x48,
        0x7F,
        0x91,
        0x7F,
        0x92,
        0x7F,
        0x92,
        0x7F,
        0x92,
        0x7F,
        0x92,
        0x7F,
        0x92,
        0x7F,
        0x91,
        0x78,
        0x48,
        0x70,
        0x44,
        0x60,
        0x20,
        0x40,
        0x10,
    },
    /*.icon_color = */ ui::Color::cyan().v,
    /*.menu_location = */ app_location_t::DEBUG,

    /*.m4_app_tag = portapack::spi_flash::image_tag_none */ {'P', 'A', 'B', 'P'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}
