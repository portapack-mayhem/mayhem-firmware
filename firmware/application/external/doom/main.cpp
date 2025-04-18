/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#include "ui_doom.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::doom {
void initialize_app(ui::NavigationView& nav) {
    nav.push<DoomView>();
}
}  // namespace ui::external_app::doom

extern "C" {

__attribute__((section(".external_app.app_doom.application_information"), used)) application_information_t _application_information_doom = {
    (uint8_t*)0x00000000,
    ui::external_app::doom::initialize_app,
    CURRENT_HEADER_VERSION,
    VERSION_MD5,
    "Doom",
    {
        0x00,
        0x00,
        0x00,
        0x00,
        0x77,
        0xDF,
        0xFF,
        0xDF,
        0xD9,
        0xFD,
        0x89,
        0xF8,
        0x89,
        0xE8,
        0x89,
        0xA8,
        0x89,
        0xA8,
        0xD9,
        0xAD,
        0x79,
        0xAF,
        0x2D,
        0xAA,
        0x07,
        0xA8,
        0x03,
        0xA0,
        0x01,
        0x80,
        0x00,
        0x00,
    },
    ui::Color::red().v,
    app_location_t::GAMES,
    -1,
    {0, 0, 0, 0},
    0x00000000,
};
}
