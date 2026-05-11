/*
 * Copyright (C) 2026
 *
 * This file is part of PortaPack / Mayhem.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#include "ui.hpp"
#include "ui_meteor_lrpt_rx.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::meteor_lrpt_rx {
void initialize_app(ui::NavigationView& nav) {
    nav.push<MeteorLrptRxView>();
}
}  // namespace ui::external_app::meteor_lrpt_rx

extern "C" {

__attribute__((section(".external_app.app_meteor_lrpt_rx.application_information"), used)) application_information_t _application_information_meteor_lrpt_rx = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::meteor_lrpt_rx::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "Meteor LRPT",
    /*.bitmap_data = */ {
        0x00, 0x1C, 0x3E, 0x7F, 0x7F, 0x3E, 0x1C, 0x08,
        0x08, 0x1C, 0x3E, 0x7F, 0x7F, 0x3E, 0x1C, 0x00,
        0x00, 0x18, 0x3C, 0x7E, 0xFF, 0x7E, 0x3C, 0x18,
        0x18, 0x3C, 0x7E, 0xFF, 0x7E, 0x3C, 0x18, 0x00,
    },
    /*.icon_color = */ ui::Color::green().v,
    /*.menu_location = */ app_location_t::RX,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_meteor_lrpt_rx */ {'P', 'M', 'L', 'R'},
    /*.m4_app_offset = */ 0x00000000,
};

}  // extern "C"
