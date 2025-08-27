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
#include "ui_game2048.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::game2048 {
void initialize_app(ui::NavigationView& nav) {
    nav.push<Game2048View>();
}
}  // namespace ui::external_app::game2048

extern "C" {

__attribute__((section(".external_app.app_game2048.application_information"), used)) application_information_t _application_information_game2048 = {
    (uint8_t*)0x00000000,
    ui::external_app::game2048::initialize_app,
    CURRENT_HEADER_VERSION,
    VERSION_MD5,

    "2048",
    {
        0x8C,
        0x31,
        0x5A,
        0x6B,
        0xDE,
        0x7B,
        0x8C,
        0x31,
        0x00,
        0x00,
        0x8C,
        0x31,
        0x5A,
        0x7B,
        0xDE,
        0x7B,
        0x8C,
        0x31,
        0x00,
        0x00,
        0x8C,
        0x31,
        0xDA,
        0x7B,
        0xDE,
        0x7B,
        0x8C,
        0x31,
        0x00,
        0x00,
        0x00,
        0x00,
    },
    ui::Color::green().v,
    app_location_t::GAMES,
    -1,

    {0, 0, 0, 0},
    0x00000000,
};
}  // namespace ui::external_app::game2048