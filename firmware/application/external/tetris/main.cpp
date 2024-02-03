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
#include "ui_tetris.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::tetris {
void initialize_app(ui::NavigationView& nav) {
    nav.push<TetrisView>();
}
}  // namespace ui::external_app::tetris

extern "C" {

__attribute__((section(".external_app.app_tetris.application_information"), used)) application_information_t _application_information_tetris = {
    /*.memory_location = */ (uint8_t*)0x00000000,  // will be filled at compile time
    /*.externalAppEntry = */ ui::external_app::tetris::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "Tetris",
    /*.bitmap_data = */ {
        0xF8,
        0xFF,
        0x88,
        0x88,
        0x88,
        0x88,
        0x88,
        0x88,
        0xF8,
        0xFF,
        0x80,
        0x08,
        0x80,
        0x08,
        0x9F,
        0x08,
        0x91,
        0x0F,
        0x11,
        0x00,
        0x11,
        0x00,
        0xFF,
        0xF1,
        0x11,
        0x91,
        0x11,
        0x91,
        0x11,
        0x91,
        0xFF,
        0xF1,
    },
    /*.icon_color = */ ui::Color::orange().v,
    /*.menu_location = */ app_location_t::UTILITIES,

    /*.m4_app_tag = portapack::spi_flash::image_tag_noop */ {'\0', '\0', '\0', '\0'},  // optional
    /*.m4_app_offset = */ 0x00000000,                                                  // will be filled at compile time
};
}
