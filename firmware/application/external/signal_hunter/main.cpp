/*
 * Copyright (C) 2026 Matej Sochan
 * This file is part of PortaPack.
 */

#include "ui.hpp"
#include "ui_signal_hunter.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::signal_hunter {
void initialize_app(ui::NavigationView& nav) {
    nav.push<SignalHunterAppView>();
}
}  // namespace ui::external_app::signal_hunter

extern "C" {
__attribute__((section(".external_app.app_signal_hunter.application_information"), used)) application_information_t _application_information_signal_hunter = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::signal_hunter::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,
    /*.app_name = */ "SigHunter",
    /*.bitmap_data = */ {
        // Target/Radar icon
        0x00, 0x00, 0x3E, 0x00, 0x41, 0x00, 0x80, 0x00, 
        0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x41, 0x00, 
        0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    },
    /*.icon_color = */ ui::Color::green().v,
    /*.menu_location = */ app_location_t::RX,
    /*.desired_menu_position = */ -1,
    
    /*.m4_app_tag = */ {'H', 'U', 'N', 'T'}, 
    /*.m4_app_offset = */ 0x00000000,
};
}
