// enhanced_drone_analyzer_main.cpp
// Main entry point for Enhanced Drone Analyzer external app
// Minimal version for modular refactoring

#include "ui_minimal_drone_analyzer.hpp"  // Enhanced version with database
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::enhanced_drone_analyzer {

void initialize_app(ui::NavigationView& nav) {
    nav.push<EnhancedDroneSpectrumAnalyzerView>();  // Step 4: Full integration
}

}  // namespace ui::external_app::enhanced_drone_analyzer

extern "C" {

__attribute__((section(".external_app.app_enhanced_drone_analyzer.application_information"), used))
application_information_t _application_information_enhanced_drone_analyzer = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::enhanced_drone_analyzer::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "Enhanced Drone Analyzer",
    /*.bitmap_data = */ {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    },
    /*.icon_color = */ ui::Color::green().v,
    /*.menu_location = */ app_location_t::RX,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_none */ {0, 0, 0, 0},
    /*.m4_app_offset = */ 0x00000000,
};

}

extern "C" {

__attribute__((section(".external_app.app_enhanced_drone_analyzer.application_information"), used))
application_information_t _application_information_enhanced_drone_analyzer = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::enhanced_drone_analyzer::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "Enhanced Drone Analyzer",
    /*.bitmap_data = */ {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    },
    /*.icon_color = */ ui::Color::green().v,
    /*.menu_location = */ app_location_t::RX,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_none */ {0, 0, 0, 0},
    /*.m4_app_offset = */ 0x00000000,
};

}
