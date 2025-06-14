/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

/*
 * Chrome Dino Game for Portapack Mayhem
 * Based on the original DinoGame by various contributors
 */

#include "ui.hpp"
#include "ui_dinogame.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::dinogame {
void initialize_app(ui::NavigationView& nav) {
    nav.push<DinoGameView>();
}
}  // namespace ui::external_app::dinogame

extern "C" {

__attribute__((section(".external_app.app_dinogame.application_information"), used)) application_information_t _application_information_dinogame = {
    (uint8_t*)0x00000000,
    ui::external_app::dinogame::initialize_app,
    CURRENT_HEADER_VERSION,
    VERSION_MD5,

    "Dino Game",
    {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0xF0,
        0xF8,
        0xFC,
        0xFE,
        0xFF,
        0xFF,
        0x7F,
        0x7F,
        0x3F,
        0x3F,
        0x3E,
        0x3C,
        0x38,
        0x30,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    },
    ui::Color::green().v,
    app_location_t::GAMES,
    -1,

    {0, 0, 0, 0},
    0x00000000,
};
}
