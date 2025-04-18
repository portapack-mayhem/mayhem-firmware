/*
 * Copyright (C) 2024 Bernd Herzog
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
#include "ui_whipcalc.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::antenna_length {
void initialize_app(ui::NavigationView& nav) {
    nav.push<WhipCalcView>();
}
}  // namespace ui::external_app::antenna_length

extern "C" {

__attribute__((section(".external_app.app_antenna_length.application_information"), used)) application_information_t _application_information_antenna_length = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::antenna_length::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "Antenna Length",
    /*.bitmap_data = */ {0x38, 0x3E, 0x10, 0x22, 0x10, 0x26, 0x10, 0x22, 0x10, 0x2E, 0x10, 0x22, 0x10, 0x26, 0x10, 0x22, 0x38, 0x2E, 0x38, 0x22, 0x38, 0x26, 0x38, 0x22, 0x38, 0x2E, 0x38, 0x22, 0x38, 0x3E, 0x00, 0x00},
    /*.icon_color = */ ui::Color::cyan().v,
    /*.menu_location = */ app_location_t::UTILITIES,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_none */ {0, 0, 0, 0},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}
