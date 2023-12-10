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
#include "ui_weatherstation.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::weatherstation {
void initialize_app(ui::NavigationView& nav) {
    nav.push<WeatherView>();
}
}  // namespace ui::external_app::weatherstation

extern "C" {

__attribute__((section(".external_app.app_weatherstation.application_information"), used)) application_information_t _application_information_weatherstation = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::weatherstation::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "Weather",
    /*.bitmap_data = */ {
        0xC0,
        0x00,
        0x20,
        0x01,
        0x10,
        0x02,
        0x10,
        0x3A,
        0x10,
        0x02,
        0x10,
        0x1A,
        0x10,
        0x02,
        0xD0,
        0x3A,
        0xD0,
        0x02,
        0xD0,
        0x1A,
        0xD0,
        0x02,
        0xE8,
        0x05,
        0xE8,
        0x05,
        0xC8,
        0x04,
        0x10,
        0x02,
        0xE0,
        0x01,
    },
    /*.icon_color = */ ui::Color::green().v,
    /*.menu_location = */ app_location_t::RX,

    /*.m4_app_tag = portapack::spi_flash::image_tag_afsk_rx */ {'P', 'W', 'T', 'H'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}
