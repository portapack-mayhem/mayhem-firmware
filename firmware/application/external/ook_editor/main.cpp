/*
 * Copyright (C) 2024 Samir Sánchez Garnica @sasaga92
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
#include "ui_ook_editor.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::ook_editor {
void initialize_app(ui::NavigationView& nav) {
    nav.push<OOKEditorAppView>();
}
}  // namespace ui::external_app::ook_editor

extern "C" {

__attribute__((section(".external_app.app_ook_editor.application_information"), used)) application_information_t _application_information_ook_editor = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::ook_editor::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "OOKEditor",
    /*.bitmap_data = */ {
        0x20,
        0x00,
        0x20,
        0x00,
        0x20,
        0x00,
        0x20,
        0x00,
        0xE0,
        0x07,
        0xF0,
        0x0F,
        0x30,
        0x0C,
        0x30,
        0x0C,
        0xF0,
        0x0F,
        0xF0,
        0x0F,
        0x70,
        0x0D,
        0xB0,
        0x0E,
        0x70,
        0x0D,
        0xB0,
        0x0E,
        0xF0,
        0x0F,
        0xE0,
        0x07,
    },
    /*.icon_color = */ ui::Color::orange().v,
    /*.menu_location = */ app_location_t::TX,

    /*.m4_app_tag = portapack::spi_flash::image_tag_ook */ {'P', 'O', 'O', 'K'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}
