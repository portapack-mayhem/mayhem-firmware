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
#include "ui_coasterp.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::coasterp {
void initialize_app(ui::NavigationView& nav) {
    nav.push<CoasterPagerView>();
}
}  // namespace ui::external_app::coasterp

extern "C" {

__attribute__((section(".external_app.app_coasterp.application_information"), used)) application_information_t _application_information_coasterp = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::coasterp::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "BurgerPgr",
    /*.bitmap_data = */ {

        0x00,
        0x00,
        0xE0,
        0x07,
        0xF8,
        0x1F,
        0xFC,
        0x3F,
        0xFE,
        0x7F,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0x00,
        0x00,
        0x55,
        0x55,
        0xAA,
        0xAA,
        0x55,
        0x55,
        0x00,
        0x00,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFE,
        0x7F,
        0x00,
        0x00,
    },
    /*.icon_color = */ ui::Color::yellow().v,
    /*.menu_location = */ app_location_t::TX,

    /*.m4_app_tag = portapack::spi_flash::image_tag_fsktx */ {'P', 'F', 'S', 'K'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}
