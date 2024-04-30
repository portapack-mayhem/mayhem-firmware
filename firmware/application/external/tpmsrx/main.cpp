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
#include "tpms_app.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::tpmsrx {
void initialize_app(ui::NavigationView& nav) {
    nav.push<TPMSAppView>();
}
}  // namespace ui::external_app::tpmsrx

extern "C" {

__attribute__((section(".external_app.app_tpmsrx.application_information"), used)) application_information_t _application_information_tpmsrx = {
    /*.memory_location = */ (uint8_t*)0x00000000,  // will be filled at compile time
    /*.externalAppEntry = */ ui::external_app::tpmsrx::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "TPMS RX",
    /*.bitmap_data = */ {
        0xC0,
        0x03,
        0xF0,
        0x0F,
        0x18,
        0x18,
        0xEC,
        0x37,
        0x36,
        0x6D,
        0x3A,
        0x59,
        0x4B,
        0xD5,
        0x8B,
        0xD3,
        0xCB,
        0xD1,
        0xAB,
        0xD2,
        0x9A,
        0x5C,
        0xB6,
        0x6C,
        0xEC,
        0x37,
        0x18,
        0x18,
        0xF0,
        0x0F,
        0xC0,
        0x03,
    },
    /*.icon_color = */ ui::Color::green().v,
    /*.menu_location = */ app_location_t::RX,

    /*.m4_app_tag = portapack::spi_flash::image_tag_tpms */ {'P', 'T', 'P', 'M'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}
