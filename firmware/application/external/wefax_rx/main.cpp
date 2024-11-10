/*
 * Copyright (C) 2024 HTotoo
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
#include "ui_wefax_rx.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::wefax_rx {
void initialize_app(ui::NavigationView& nav) {
    nav.push<WeFaxRxView>();
}
}  // namespace ui::external_app::wefax_rx

extern "C" {

__attribute__((section(".external_app.app_wefax_rx.application_information"), used)) application_information_t _application_information_wefax_rx = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::wefax_rx::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "WeFax",
    /*.bitmap_data = */ {
        0x00,
        0x00,
        0xFC,
        0x07,
        0x04,
        0x0C,
        0x04,
        0x1C,
        0x04,
        0x3C,
        0x84,
        0x21,
        0x84,
        0x21,
        0x84,
        0x21,
        0xF4,
        0x2F,
        0xF4,
        0x2F,
        0x84,
        0x21,
        0x84,
        0x21,
        0x84,
        0x21,
        0x04,
        0x20,
        0xFC,
        0x3F,
        0x00,
        0x00,
    },
    /*.icon_color = */ ui::Color::orange().v,
    /*.menu_location = */ app_location_t::RX,

    /*.m4_app_tag = portapack::spi_flash::image_tag_wefaxrx */ {'P', 'W', 'F', 'X'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}
