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
#include "ui_fmradio.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::fmradio {
void initialize_app(ui::NavigationView& nav) {
    nav.push<FmRadioView>();
}
}  // namespace ui::external_app::fmradio

extern "C" {

__attribute__((section(".external_app.app_fmradio.application_information"), used)) application_information_t _application_information_fmradio = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::fmradio::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "FM Radio",
    /*.bitmap_data = */ {
        0x00,
        0x00,
        0x00,
        0x00,
        0x04,
        0x20,
        0x12,
        0x48,
        0x8A,
        0x51,
        0xCA,
        0x53,
        0xCA,
        0x53,
        0x8A,
        0x51,
        0x12,
        0x48,
        0x84,
        0x21,
        0xC0,
        0x03,
        0x40,
        0x02,
        0x60,
        0x06,
        0x20,
        0x04,
        0x30,
        0x0C,
        0xF0,
        0x0F,
    },
    /*.icon_color = */ ui::Color::green().v,
    /*.menu_location = */ app_location_t::RX,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_wfm_audio */ {'P', 'W', 'F', 'M'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}
