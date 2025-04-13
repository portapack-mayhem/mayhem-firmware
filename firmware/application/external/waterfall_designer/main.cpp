/*
 * Copyright (C) 2025 Mark Thompson
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
#include "ui_waterfall_designer.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::waterfall_designer {
void initialize_app(ui::NavigationView& nav) {
    nav.push<WaterfallDesignerView>();
}
}  // namespace ui::external_app::waterfall_designer

extern "C" {

__attribute__((section(".external_app.app_waterfall_designer.application_information"), used)) application_information_t _application_information_waterfall_designer = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::waterfall_designer::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "Wtf Design",
    /*.bitmap_data = */ {
        0x00,
        0x00,
        0xC0,
        0x01,
        0x80,
        0x00,
        0x80,
        0x20,
        0x60,
        0x13,
        0x10,
        0x0C,
        0x88,
        0x08,
        0x84,
        0x10,
        0x84,
        0x10,
        0xC2,
        0x21,
        0x84,
        0x10,
        0x04,
        0x10,
        0x08,
        0x08,
        0x10,
        0x04,
        0x60,
        0x03,
        0x80,
        0x00,
    },
    /*.icon_color = */ ui::Color::cyan().v,
    /*.menu_location = */ app_location_t::RX,
    /*.desired_menu_position = */ 0,

    /*.m4_app_tag = portapack::spi_flash::image_tag_none */ {'P', 'C', 'A', 'P'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}