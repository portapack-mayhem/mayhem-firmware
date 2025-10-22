/*
 * Copyright (C) 2025 Pezsma
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
#include "ui_adult_toys_controller.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::adult_toys_controller {

void initialize_app(::ui::NavigationView& nav) {
    nav.push<AdultToysView>();
}

}  // namespace ui::external_app::adult_toys_controller

extern "C" {

__attribute__((section(".external_app.app_adult_toys_controller.application_information"), used)) application_information_t _application_information_adult_toys_controller = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::adult_toys_controller::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "Adult Toys",
    /*.bitmap_data = */ {
        0xC7,
        0xF1,
        0x81,
        0xC0,
        0x01,
        0xC0,
        0x10,
        0x84,
        0x38,
        0x8E,
        0x38,
        0x8E,
        0xF8,
        0x8F,
        0xF8,
        0x8F,
        0xF1,
        0xC7,
        0xE1,
        0xC7,
        0xE3,
        0xC3,
        0xC3,
        0xE1,
        0x87,
        0xF0,
        0x0F,
        0xF8,
        0x1F,
        0xFC,
        0x3F,
        0xFE,
    },
    /*.icon_color = */ ui::Color::yellow().v,
    /*.menu_location = */ app_location_t::TX,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_btle_tx */ {'P', 'B', 'T', 'T'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}
