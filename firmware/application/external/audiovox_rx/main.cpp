/*
 * Copyright (C) 2026 PortaPack Mayhem
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#include "ui.hpp"
#include "ui_audiovox_rx.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::audiovox_rx {

void initialize_app(ui::NavigationView& nav) {
    nav.push<AudioVoxRxView>();
}

}  // namespace ui::external_app::audiovox_rx

extern "C" {

__attribute__((section(".external_app.app_audiovox_rx.application_information"), used)) application_information_t _application_information_audiovox_rx = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::audiovox_rx::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,
    /*.app_name = */ "AudioVox RX",
    /*.bitmap_data = */ {
        0x00, 0x00, 0x40, 0x10, 0x60, 0x20, 0x70, 0x44,
        0x78, 0x48, 0x7F, 0x91, 0x7F, 0x92, 0x7F, 0x92,
        0x7F, 0x92, 0x7F, 0x92, 0x7F, 0x92, 0x7F, 0x91,
        0x78, 0x48, 0x70, 0x44, 0x60, 0x20, 0x40, 0x10,
    },
    /*.icon_color = */ ui::Color::green().v,
    /*.menu_location = */ app_location_t::RX,
    /*.desired_menu_position = */ -1,
    /* The bundled AM audio image is the largest baseband this app can run. */
    /*.m4_app_tag = portapack::spi_flash::image_tag_am_audio */ {'P', 'A', 'M', 'A'},
    /*.m4_app_offset = */ 0x00000000,
};

}  // extern "C"
