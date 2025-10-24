// enhanced_drone_analyzer_main.cpp
// Entry point for the modular Enhanced Drone Analyzer application
// This file creates and connects all the modular components

#include "ui_minimal_drone_analyzer.hpp"  // Now references the modular version
//#include "ui_drone_loading_screen.hpp"    // Loading screen UI - merged into ui_drone_ui.hpp
// #include "ui_navigation.hpp"  // REMOVED: Portapack UI framework doesn't use this
// #include "ui_territory_blocker.hpp"  // REMOVED: Files deleted, functionality removed
#include "external_app.hpp"
#include "spi_image.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

void initialize_app(ui::NavigationView& nav) {
    // Show loading screen first, then main view auto-appears after 1 second
    nav.push<LoadingScreenView>();
}

}  // namespace ui::external_app::enhanced_drone_analyzer

extern "C" {

__attribute__((section(".external_app.app_enhanced_drone_analyzer.application_information"), used))
application_information_t _application_information_enhanced_drone_analyzer = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::enhanced_drone_analyzer::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "EDA v0.3",
    /*.bitmap_data = */ {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    },
    /*.icon_color = */ ui::Color::green().v,
    /*.menu_location = */ app_location_t::RX,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_wideband_spectrum */ {'P', 'S', 'P', 'E'},
    /*.m4_app_offset = */ 0x00000000,
};

}
