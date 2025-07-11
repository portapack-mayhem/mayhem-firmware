/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#include "ui.hpp"
#include "ui_breakout.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::breakout {
void initialize_app(ui::NavigationView& nav) {
    nav.push<BreakoutView>();
}
}  // namespace ui::external_app::breakout

extern "C" {

__attribute__((section(".external_app.app_breakout.application_information"), used)) application_information_t _application_information_breakout = {
    (uint8_t*)0x00000000,
    ui::external_app::breakout::initialize_app,
    CURRENT_HEADER_VERSION,
    VERSION_MD5,

    "Breakout",
    {
        0x00,
        0x00,
        0x7F,
        0x7F,
        0x7F,
        0x7F,
        0x7F,
        0x7F,
        0x00,
        0x00,
        0xF7,
        0xF7,
        0xF7,
        0xF7,
        0xF7,
        0xF7,
        0x00,
        0x00,
        0x7F,
        0x7F,
        0x7F,
        0x7F,
        0x7F,
        0x7F,
        0x00,
        0x00,
        0xF7,
        0xF7,
        0xF7,
        0xF7,
        0xF7,
        0xF7,
    },
    ui::Color::green().v,
    app_location_t::GAMES,
    -1,

    {0, 0, 0, 0},
    0x00000000,
};
}  // namespace ui::external_app::breakout