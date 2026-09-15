/*
 * Copyright (C) 2026 Dmytro Onyshko
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
#include "ui_ft8_rx.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::ft8_rx {
void initialize_app(ui::NavigationView& nav) {
    nav.push<FT8RxView>();
}
}  // namespace ui::external_app::ft8_rx

extern "C" {

__attribute__((section(".external_app.app_ft8_rx.application_information"), used)) application_information_t _application_information_ft8_rx = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::ft8_rx::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "FT8",
    /*.bitmap_data = */
    {
        0x00, 0x00,
        0x00, 0x00,
        0x08, 0x10,
        0x14, 0x28,
        0x22, 0x44,
        0x41, 0x82,
        0x81, 0x81,
        0x81, 0x81,
        0x41, 0x82,
        0x22, 0x44,
        0x14, 0x28,
        0x08, 0x10,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
    },
    /*.icon_color = */ ui::Color::blue().v,
    /*.menu_location = */ app_location_t::RX,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_ft8_rx */ {'P', 'F', 'T', '8'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}
