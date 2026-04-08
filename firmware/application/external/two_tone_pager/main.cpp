/*
 * Copyright (C) 2024 PortaPack Mayhem
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
#include "ui_two_tone_pager.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::two_tone_pager {
void initialize_app(ui::NavigationView& nav) {
    nav.push<TwoTonePagerView>();
}
}  // namespace ui::external_app::two_tone_pager

extern "C" {

__attribute__((section(".external_app.app_two_tone_pager.application_information"), used)) application_information_t _application_information_two_tone_pager = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::two_tone_pager::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "2-Tone TX",
    /*.bitmap_data = */ {
        // 16×16 pager icon — two-tone radio / pager device
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0xFC,
        0x3F,
        0xFE,
        0x7F,
        0x02,
        0x40,
        0xBA,
        0x45,
        0x02,
        0x40,
        0xFE,
        0x7F,
        0xFE,
        0x7F,
        0x92,
        0x7C,
        0x92,
        0x7C,
        0xFC,
        0x3F,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    },
    /*.icon_color = */ ui::Color::orange().v,
    /*.menu_location = */ app_location_t::TX,
    /*.desired_menu_position = */ -1,

    // Uses the proc_tones baseband processor (same as Morse TX)
    /*.m4_app_tag = portapack::spi_flash::image_tag_tones */ {'P', 'T', 'O', 'N'},
    /*.m4_app_offset = */ 0x00000000,  // filled at compile time
};

}  // extern "C"
