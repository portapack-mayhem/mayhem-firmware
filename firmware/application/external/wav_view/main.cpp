/*
 * Copyright (C) 2024 Bernd Herzog
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
#include "ui_view_wav.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::view_wav {
void initialize_app(ui::NavigationView& nav) {
    nav.push<ViewWavView>();
}
}  // namespace ui::external_app::view_wav

extern "C" {

__attribute__((section(".external_app.app_view_wav.application_information"), used)) application_information_t _application_information_view_wav = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::view_wav::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "WAV Viewer",
    /*.bitmap_data = */ {0xF0, 0x0F, 0x1C, 0x18, 0x17, 0x38, 0x15, 0x78, 0x15, 0xF8, 0x15, 0x82, 0x15, 0x8B, 0xD5, 0x83, 0xD5, 0xBB, 0xD5, 0x83, 0x15, 0x8B, 0x15, 0x92, 0x15, 0xA0, 0x17, 0x80, 0x1C, 0x80, 0xF0, 0xFF},
    /*.icon_color = */ ui::Color::green().v,
    /*.menu_location = */ app_location_t::UTILITIES,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_none */ {'P', 'A', 'T', 'X'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}
