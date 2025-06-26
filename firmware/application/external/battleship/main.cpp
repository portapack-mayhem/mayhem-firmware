/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#include "ui.hpp"
#include "ui_battleship.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::battleship {
void initialize_app(ui::NavigationView& nav) {
    nav.push<BattleshipView>();
}
}  // namespace ui::external_app::battleship

extern "C" {

__attribute__((section(".external_app.app_battleship.application_information"), used)) application_information_t _application_information_battleship = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::battleship::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "Battleship",
    /*.bitmap_data = */ {
        // Ship icon
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x1C,
        0x38,
        0x3E,
        0x7C,
        0x7F,
        0xFE,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0x7F,
        0xFE,
        0x3F,
        0xFC,
        0x1F,
        0xF8,
        0x0F,
        0xF0,
        0x07,
        0xE0,
        0x03,
        0xC0,
        0x01,
        0x80,
    },
    /*.icon_color = */ ui::Color::blue().v,
    /*.menu_location = */ app_location_t::GAMES,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = */ {'P', 'F', 'S', 'R'},  // Use FSK RX baseband
    /*.m4_app_offset = */ 0x00000000,         // will be filled at compile time
};
}