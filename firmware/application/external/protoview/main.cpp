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
#include "ui_protoview.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::protoview {
void initialize_app(ui::NavigationView& nav) {
    nav.push<ProtoView>();
}
}  // namespace ui::external_app::protoview

extern "C" {

__attribute__((section(".external_app.app_protoview.application_information"), used)) application_information_t _application_information_protoview = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::protoview::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "ProtoView",
    /*.bitmap_data = */ {
        0x00,
        0x00,
        0x00,
        0x00,
        0xF8,
        0x87,
        0x08,
        0x84,
        0x0F,
        0xFC,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0xF3,
        0xE0,
        0x92,
        0xA0,
        0x9E,
        0xBF,
        0x00,
        0x00,
        0x00,
        0x00,
        0xFC,
        0xF3,
        0x04,
        0x12,
        0x07,
        0x1E,
    },
    /*.icon_color = */ ui::Color::orange().v,
    /*.menu_location = */ app_location_t::RX,

    /*.m4_app_tag = portapack::spi_flash::image_tag_protoview */ {'P', 'P', 'V', 'W'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}
