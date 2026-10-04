/*
 * Copyright (C) 2026 SecLBL
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
#include "ui_music.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::music {
void initialize_app(ui::NavigationView& nav) {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());
    nav.push<MusicView>();
}
}  // namespace ui::external_app::music

extern "C" {

__attribute__((section(".external_app.app_music.application_information"), used)) application_information_t _application_information_music = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::music::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "Music",
    /*.bitmap_data = */ {
        0x00,
        0x00,
        0xC0,
        0x1F,
        0xC0,
        0x1F,
        0x40,
        0x10,
        0x40,
        0x10,
        0x40,
        0x10,
        0x40,
        0x10,
        0x40,
        0x10,
        0x40,
        0x10,
        0x40,
        0x10,
        0x70,
        0x1C,
        0x78,
        0x1E,
        0x78,
        0x1E,
        0x30,
        0x0C,
        0x00,
        0x00,
        0x00,
        0x00,
    },
    /*.icon_color = */ ui::Color::red().v,
    /*.menu_location = */ app_location_t::UTILITIES,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = audio_play (baseband/proc_audio_play.cpp) */ {'P', 'A', 'P', 'L'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}
