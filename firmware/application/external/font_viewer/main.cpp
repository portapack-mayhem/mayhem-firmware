/*
 * Copyright (C) 2023 Mark Thompson
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
#include "ui_font_viewer.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::font_viewer {
void initialize_app(ui::NavigationView& nav) {
    nav.push<DebugFontsView>();
}
}  // namespace ui::external_app::font_viewer

extern "C" {

__attribute__((section(".external_app.app_font_viewer.application_information"), used)) application_information_t _application_information_font_viewer = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::font_viewer::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "Font Viewer",
    /*.bitmap_data = */ {
        0x00,
        0x00,
        0x00,
        0x00,
        0x10,
        0x0C,
        0x38,
        0x0C,
        0x38,
        0x0C,
        0x6C,
        0x0C,
        0x6C,
        0x0C,
        0xC6,
        0x7C,
        0xFE,
        0xFC,
        0xFF,
        0x8D,
        0x83,
        0x8D,
        0x83,
        0xFD,
        0x01,
        0x7D,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    },
    /*.icon_color = */ ui::Color::cyan().v,
    /*.menu_location = */ app_location_t::DEBUG,

    /*.m4_app_tag = portapack::spi_flash::image_tag_none */ {0, 0, 0, 0},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}
