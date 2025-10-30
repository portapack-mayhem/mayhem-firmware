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
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::breakout::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "Breakout",
    /*.bitmap_data = */ {
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
    /*.icon_color = */ ui::Color::green().v,
    /*.menu_location = */ app_location_t::GAMES,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = */ {'P', 'A', 'B', 'P'},  // This is the audio beep baseband tag!
    /*.m4_app_offset = */ 0x00000000,         // will be filled at compile time
};
}