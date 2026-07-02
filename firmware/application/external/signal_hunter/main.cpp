/*
 * Copyright (C) 2026 Matej Sochan
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
#include "ui_signal_hunter.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::signal_hunter {
void initialize_app(ui::NavigationView& nav) {
    nav.push<SignalHunterAppView>();
}
}  // namespace ui::external_app::signal_hunter

extern "C" {
__attribute__((section(".external_app.app_signal_hunter.application_information"), used)) application_information_t _application_information_signal_hunter = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::signal_hunter::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,
    /*.app_name = */ "SigHunter",
    /*.bitmap_data = */ {
    0x80, 0x01,
    0x80, 0x01,
    0xE0, 0x07,
    0xF0, 0x0F,
    0x98, 0x19,
    0x8C, 0x31,
    0x84, 0x21,
    0xFF, 0xFF,
    0xFF, 0xFF,
    0x84, 0x21,
    0x8C, 0x31,
    0x98, 0x19,
    0xF0, 0x0F,
    0xE0, 0x07,
    0x80, 0x01,
    0x80, 0x01
    }, 
    /*.icon_color = */ ui::Color::green().v,
    /*.menu_location = */ app_location_t::RX,
    /*.desired_menu_position = */ -1,
    
    /*.m4_app_tag = */ {'H', 'U', 'N', 'T'}, 
    /*.m4_app_offset = */ 0x00000000,
};
}
