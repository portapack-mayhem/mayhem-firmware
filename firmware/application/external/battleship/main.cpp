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
        // Pirate galleon - 16x16
        0x80, 0x00,  // ........#.......
        0x80, 0x00,  // ........#.......
        0x80, 0x00,  // ........#.......
        0xC0, 0x01,  // .......###......
        0xE0, 0x03,  // ......#####.....
        0xF0, 0x07,  // .....#######....
        0xF8, 0x0F,  // ....#########...
        0xFC, 0x1F,  // ...###########..
        0xFE, 0x3F,  // ..#############.
        0x00, 0x01,  // .......#........
        0x00, 0x01,  // .......#........
        0x00, 0x01,  // .......#........
        0xFC, 0x3F,  // ..############..
        0xFE, 0x7F,  // .##############.
        0xFF, 0xFF,  // ################
        0xFC, 0x3F,  // ..############..
    },
    /*.icon_color = */ ui::Color::blue().v,
    /*.menu_location = */ app_location_t::GAMES,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = */ {'P', 'P', 'O', '2'},  // Use POCSAG2 baseband (larger than FSKTX)
    /*.m4_app_offset = */ 0x00000000,         // will be filled at compile time
};
}
