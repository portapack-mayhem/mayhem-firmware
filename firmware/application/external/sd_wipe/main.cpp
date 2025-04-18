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
#include "ui_sd_wipe.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::sd_wipe {
void initialize_app(ui::NavigationView& nav) {
    nav.push<WipeSDView>();
}
}  // namespace ui::external_app::sd_wipe

extern "C" {

__attribute__((section(".external_app.app_sd_wipe.application_information"), used)) application_information_t _application_information_sd_wipe = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::sd_wipe::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "Wipe SD card",
    /*.bitmap_data = */ {0xF0, 0x3F, 0x58, 0x35, 0x5C, 0x35, 0xFC, 0x3F, 0xFC, 0x3F, 0xFC, 0x3F, 0x3C, 0x1C, 0xBC, 0xC9, 0xBC, 0xE3, 0x2C, 0x77, 0x5C, 0x3E, 0xAC, 0x1C, 0x5C, 0x3E, 0x2C, 0x77, 0x9C, 0xE3, 0xAC, 0xC1},
    /*.icon_color = */ ui::Color::red().v,
    /*.menu_location = */ app_location_t::UTILITIES,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_none */ {0, 0, 0, 0},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}
