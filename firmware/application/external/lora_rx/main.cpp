/*
 * LoRa RX - MeshCore receiver
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#include "ui.hpp"
#include "ui_lora_rx.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::lora_rx {
void initialize_app(ui::NavigationView& nav) {
    nav.push<LoraRxView>();
}
}  // namespace ui::external_app::lora_rx

extern "C" {

__attribute__((section(".external_app.app_lora_rx.application_information"), used)) application_information_t _application_information_lora_rx = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::lora_rx::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "MeshCore RX",
    /*.bitmap_data = */ {
        // three linked nodes
        0x00,
        0x00,
        0x18,
        0x00,
        0x3C,
        0x00,
        0x3C,
        0x00,
        0x18,
        0x18,
        0x24,
        0x3C,
        0x42,
        0x3C,
        0x81,
        0x18,
        0x81,
        0x00,
        0x42,
        0x00,
        0x24,
        0x18,
        0x18,
        0x3C,
        0x3C,
        0x3C,
        0x3C,
        0x18,
        0x18,
        0x00,
        0x00,
        0x00,
    },
    /*.icon_color = */ ui::Color::green().v,
    /*.menu_location = */ app_location_t::RX,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_lora_rx */ {'P', 'L', 'R', 'A'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}
