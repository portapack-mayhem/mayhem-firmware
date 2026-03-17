/*
 * Copyright (C) 2025 Sarah Rose
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
#include "ui_p25_tx.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::p25_tx {
void initialize_app(ui::NavigationView& nav) {
    nav.push<P25TxView>();
}
}  // namespace ui::external_app::p25_tx

extern "C" {

__attribute__((section(".external_app.app_p25_tx.application_information"), used))
application_information_t _application_information_p25_tx = {
    /*.memory_location    = */ (uint8_t*)0x00000000,
    /*.externalAppEntry   = */ ui::external_app::p25_tx::initialize_app,
    /*.header_version     = */ CURRENT_HEADER_VERSION,
    /*.app_version        = */ VERSION_MD5,

    /*.app_name           = */ "P25 TX",
    /*.bitmap_data        = */ {
        0x00,
        0x00,
        0xFE,
        0x7F,
        0x02,
        0x40,
        0xFA,
        0x5F,
        0x0A,
        0x50,
        0x0A,
        0x50,
        0xFA,
        0x5F,
        0x02,
        0x40,
        0x02,
        0x40,
        0xE2,
        0x47,
        0x22,
        0x44,
        0xE2,
        0x47,
        0x02,
        0x40,
        0x02,
        0x40,
        0xFE,
        0x7F,
        0x00,
        0x00,
    },
    /*.icon_color         = */ ui::Color::orange().v,
    /*.menu_location      = */ app_location_t::TX,
    /*.desired_menu_position = */ -1,
    /*.m4_app_tag         = */ {'P', '2', '5', 'T'},
    /*.m4_app_offset      = */ 0x00000000,
};
}
