/*
 * SAME TX - Specific Area Message Encoding Transmitter
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
 */

#include "ui.hpp"
#include "ui_same_tx.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::same_tx {
void initialize_app(ui::NavigationView& nav) {
    nav.push<SameTxView>();
}
}  // namespace ui::external_app::same_tx

extern "C" {

__attribute__((section(".external_app.app_same_tx.application_information"), used)) application_information_t _application_information_same_tx = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::same_tx::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "SAME TX",
    /*.bitmap_data = */ {
        // 16x16 icon - simple "!" alert symbol
        0x00,
        0x00,
        0x18,
        0x00,
        0x3C,
        0x00,
        0x3C,
        0x00,
        0x3C,
        0x00,
        0x3C,
        0x00,
        0x18,
        0x00,
        0x18,
        0x00,
        0x00,
        0x00,
        0x18,
        0x00,
        0x3C,
        0x00,
        0x3C,
        0x00,
        0x18,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    },
    /*.icon_color = */ ui::Color::yellow().v,
    /*.menu_location = */ app_location_t::TX,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_afsk */ {'P', 'A', 'F', 'T'},
    /*.m4_app_offset = */ 0x00000000,
};
}
