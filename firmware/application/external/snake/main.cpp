/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#include "ui.hpp"
#include "ui_snake.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::snake {
void initialize_app(ui::NavigationView& nav) {
    nav.push<SnakeView>();
}
}  // namespace ui::external_app::snake

extern "C" {

__attribute__((section(".external_app.app_snake.application_information"), used)) application_information_t _application_information_snake = {
    (uint8_t*)0x00000000,
    ui::external_app::snake::initialize_app,
    CURRENT_HEADER_VERSION,
    VERSION_MD5,
    "Snake",
    {
        0x00,
        0x00,
        0x00,
        0x00,
        0xE0,
        0x09,
        0x70,
        0xC7,
        0xFC,
        0xC9,
        0x06,
        0x00,
        0x06,
        0x00,
        0x0C,
        0x00,
        0xF0,
        0x01,
        0x00,
        0x3E,
        0x00,
        0x40,
        0xFC,
        0x40,
        0x02,
        0x3F,
        0x02,
        0x00,
        0x7C,
        0x80,
        0x80,
        0x7F,
    },
    ui::Color::green().v,
    app_location_t::GAMES,
    -1,
    {0, 0, 0, 0},
    0x00000000,
};
}  // namespace ui::external_app::snake