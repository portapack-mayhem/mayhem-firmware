/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#include "ui.hpp"
#include "ui_spaceinv.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::spaceinv {
void initialize_app(ui::NavigationView& nav) {
    nav.push<SpaceInvadersView>();
}
}  // namespace ui::external_app::spaceinv

extern "C" {

__attribute__((section(".external_app.app_spaceinv.application_information"), used)) application_information_t _application_information_spaceinv = {
    (uint8_t*)0x00000000,
    ui::external_app::spaceinv::initialize_app,
    CURRENT_HEADER_VERSION,
    VERSION_MD5,

    "Space Invaders",
    {
        0x18,
        0x3C,
        0x7E,
        0xDB,
        0xFF,
        0xFF,
        0x24,
        0x66,
        0x42,
        0x00,
        0x24,
        0x18,
        0x24,
        0x42,
        0x81,
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
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    },
    ui::Color::yellow().v,
    app_location_t::GAMES,
    -1,

    {0, 0, 0, 0},
    0x00000000,
};
}  // namespace ui::external_app::spaceinv
