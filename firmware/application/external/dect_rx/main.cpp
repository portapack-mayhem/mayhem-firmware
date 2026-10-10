/*
 * DECT RX external application entry point for the PortaPack Mayhem DECT RX.
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
#include "ui_dect_rx.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::dect_rx {
void initialize_app(ui::NavigationView& nav) {
    nav.push<DECTRxView>();
}
}  // namespace ui::external_app::dect_rx

extern "C" {

__attribute__((section(".external_app.app_dect_rx.application_information"), used)) application_information_t _application_information_dect_rx = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::dect_rx::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "DECT",
    /*.bitmap_data = */ {
        0x00,
        0x00,
        0xF0,
        0x0F,
        0xD0,
        0x0B,
        0x10,
        0x08,
        0x10,
        0x08,
        0x10,
        0x08,
        0x10,
        0x08,
        0x10,
        0x08,
        0x10,
        0x08,
        0x10,
        0x08,
        0x10,
        0x08,
        0x10,
        0x08,
        0xF0,
        0x0F,
        0x90,
        0x09,
        0xF0,
        0x0F,
        0x00,
        0x00,
    },
    /*.icon_color = */ ui::Color::green().v,
    /*.menu_location = */ app_location_t::RX,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_dect */ {'P', 'D', 'E', 'C'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}
