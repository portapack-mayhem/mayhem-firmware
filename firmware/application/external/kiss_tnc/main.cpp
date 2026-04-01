/*
 * Copyright (C) 2024 Sarah Rose
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
#include "ui_kiss_tnc.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::kiss_tnc {
void initialize_app(ui::NavigationView& nav) {
    nav.push<KissTncView>();
}
}  // namespace ui::external_app::kiss_tnc

extern "C" {

__attribute__((section(".external_app.app_kiss_tnc.application_information"), used)) application_information_t _application_information_kiss_tnc = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::kiss_tnc::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "KISS TNC",
    /*.bitmap_data = */ {
        // 16x16 icon - radio tower / TNC
        0x00,
        0x00,
        0x08,
        0x00,
        0x14,
        0x00,
        0x22,
        0x00,
        0x08,
        0x00,
        0x08,
        0x00,
        0x08,
        0x00,
        0x3E,
        0x00,
        0x22,
        0x00,
        0x22,
        0x00,
        0x22,
        0x00,
        0x22,
        0x00,
        0x3E,
        0x00,
        0x1C,
        0x00,
        0x08,
        0x00,
        0x00,
        0x00,
    },
    /*.icon_color = */ ui::Color::green().v,
    /*.menu_location = */ app_location_t::TRX,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_aprs_rx */ {'P', 'A', 'P', 'R'},
    /*.m4_app_offset = */ 0x00000000,
};
}
