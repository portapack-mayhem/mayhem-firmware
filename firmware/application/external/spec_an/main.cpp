/*
 * Copyright (C) 2026 zxkmm
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
#include "ui_spec_an.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::spec_an {
void initialize_app(ui::NavigationView& nav) {
    nav.push<SpecAnView>();
}
}  // namespace ui::external_app::spec_an

extern "C" {

__attribute__((section(".external_app.app_spec_an.application_information"), used)) application_information_t _application_information_spec_an = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::spec_an::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "Spectrum An",
    /*.bitmap_data = */ {
        0xF8,
        0x01,
        0xFC,
        0x03,
        0x0E,
        0x07,
        0x07,
        0x0E,
        0xF3,
        0x0C,
        0x9F,
        0x0F,
        0x9F,
        0x0F,
        0xF3,
        0x0C,
        0x07,
        0x0E,
        0x0E,
        0x07,
        0xFC,
        0x1F,
        0xF8,
        0x3D,
        0x00,
        0x7C,
        0x00,
        0xF8,
        0x00,
        0xF0,
        0x00,
        0x60,
    },
    /*.icon_color = */ ui::Color::green().v,
    /*.menu_location = */ app_location_t::RX,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_spec_an */ {'P', 'S', 'A', 'N'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}
