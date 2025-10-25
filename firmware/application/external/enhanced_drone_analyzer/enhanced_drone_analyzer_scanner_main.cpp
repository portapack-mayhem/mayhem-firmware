/*
 * Enhanced Drone Analyzer Scanner App - Modular Implementation
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
#include "ui_navigation.hpp"
#include "external_app.hpp"
#include "ui_scanner_combined.hpp"

using namespace ui::external_app::enhanced_drone_analyzer;

namespace ui::external_app::enhanced_drone_analyzer_scanner {
void initialize_app(ui::NavigationView& nav) {
    nav.push<EnhancedDroneSpectrumAnalyzerView>();
}
}  // namespace ui::external_app::enhanced_drone_analyzer_scanner

extern "C" {

__attribute__((section(".external_app.app_enhanced_drone_analyzer_scanner.application_information"), used)) application_information_t _application_information_enhanced_drone_analyzer_scanner = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::enhanced_drone_analyzer_scanner::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "EDA Scanner",
    /*.bitmap_data = */ {
        0x00, 0x00, 0x80, 0x01, 0xC0, 0x03, 0xE0, 0x07, 0xF0, 0x0F, 0xF8, 0x1F,
        0xFC, 0x3F, 0xFE, 0x7F, 0xFE, 0x7F, 0xFC, 0x3F, 0xF8, 0x1F, 0xF0, 0x0F,
        0xE0, 0x07, 0xC0, 0x03, 0x80, 0x01, 0x00, 0x00
    },
    /*.icon_color = */ ui::Color::red().v,
    /*.menu_location = */ app_location_t::RX,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = */ {'E', 'D', 'A', 'S'},  // EDA Scanner
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}
