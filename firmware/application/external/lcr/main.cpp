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
#include "ui_lcr.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::lcr {
void initialize_app(ui::NavigationView& nav) {
    nav.push<LCRView>();
}
}  // namespace ui::external_app::lcr

extern "C" {

__attribute__((section(".external_app.app_lcr.application_information"), used)) application_information_t _application_information_lcr = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::lcr::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "TEDI/LCR",
    /*.bitmap_data = */ {
        0x0C,
        0x00,
        0xFF,
        0x7F,
        0x01,
        0x80,
        0xC1,
        0x9B,
        0xFF,
        0x7F,
        0x0C,
        0x00,
        0xFF,
        0x7F,
        0x01,
        0x80,
        0xC1,
        0x9D,
        0xFF,
        0x7F,
        0x0C,
        0x00,
        0x0C,
        0x00,
        0x0C,
        0x00,
        0x0C,
        0x00,
        0x0C,
        0x00,
        0x0C,
        0x00,
    },
    /*.icon_color = */ ui::Color::yellow().v,
    /*.menu_location = */ app_location_t::TX,

    /*.m4_app_tag = portapack::spi_flash::image_tag_afsk */ {'P', 'A', 'F', 'T'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}
