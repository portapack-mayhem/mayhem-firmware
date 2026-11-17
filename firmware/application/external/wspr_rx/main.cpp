/*
 * Copyright (C) 2026 Ivan
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
#include "ui_navigation.hpp"
#include "ui_wspr_rx.hpp"
#include "external_app.hpp"

namespace ui::external_app::wspr_rx {
void initialize_app(ui::NavigationView& nav) {
    nav.push<WSPRRxView>();
}
}  // namespace ui::external_app::wspr_rx

extern "C" {

__attribute__((section(".external_app.app_wspr_rx.application_information"), used)) application_information_t _application_information_wspr_rx = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::wspr_rx::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "WSPR",
    /* A letter W beside four rising signal bars on a baseline: the beacon-report
     * protocol this app listens for. */
    /*.bitmap_data = */ {
        0x00,
        0x00,
        0x00,
        0x01,
        0x00,
        0x01,
        0x00,
        0x01,
        0x00,
        0x05,
        0x88,
        0x81,
        0x88,
        0x85,
        0x88,
        0x95,
        0x55,
        0x15,
        0x55,
        0x15,
        0x22,
        0x55,
        0x00,
        0x55,
        0x00,
        0x55,
        0x00,
        0x55,
        0x00,
        0x7F,
        0x00,
        0x00,
    },
    /*.icon_color = */ ui::Color::yellow().v,
    /*.menu_location = */ app_location_t::RX,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_wspr_rx */ {'P', 'W', 'S', 'R'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}
