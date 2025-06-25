/*
 * Blackjack Game for Portapack Mayhem
 * Ported / Enhanced / Graphically made awesome by RocketGod (https://betaskynet.com)
 * Based on BlackJack 83 for TI Calculator by Harper Maddox (was written in Assembly)
 */

#include "ui.hpp"
#include "ui_blackjack.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::blackjack {
void initialize_app(ui::NavigationView& nav) {
    nav.push<BlackjackView>();
}
}  // namespace ui::external_app::blackjack

extern "C" {

__attribute__((section(".external_app.app_blackjack.application_information"), used)) application_information_t _application_information_blackjack = {
    (uint8_t*)0x00000000,
    ui::external_app::blackjack::initialize_app,
    CURRENT_HEADER_VERSION,
    VERSION_MD5,

    "Blackjack",
    {
        // Diamond icon 16x16
        0x00, 0x00,  // ................
        0x80, 0x01,  // .......##.......
        0xC0, 0x03,  // ......####......
        0xE0, 0x07,  // .....######.....
        0xF0, 0x0F,  // ....########....
        0xF8, 0x1F,  // ...##########...
        0xFC, 0x3F,  // ..############..
        0xFE, 0x7F,  // .##############.
        0xFE, 0x7F,  // .##############.
        0xFC, 0x3F,  // ..############..
        0xF8, 0x1F,  // ...##########...
        0xF0, 0x0F,  // ....########....
        0xE0, 0x07,  // .....######.....
        0xC0, 0x03,  // ......####......
        0x80, 0x01,  // .......##.......
        0x00, 0x00,  // ................
    },
    ui::Color::red().v,  // Red color for diamonds
    app_location_t::GAMES,
    -1,

    {0, 0, 0, 0},
    0x00000000,
};
}
