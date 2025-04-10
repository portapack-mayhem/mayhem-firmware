/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#include "ui.hpp"
#include "ui_flex_rx.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::flex_rx {
void initialize_app(ui::NavigationView& nav) {
    nav.push<FlexRxView>();
}
}  // namespace ui::external_app::flex_rx

extern "C" {

__attribute__((section(".external_app.app_flex_rx.application_information"), used)) application_information_t _application_information_flex_rx = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::flex_rx::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "Flex RX",
    /*.bitmap_data = */ {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0xF8,
        0x1F,
        0x04,
        0x20,
        0x02,
        0x40,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xAB,
        0xDF,
        0xAB,
        0xDF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    },
    /*.icon_color = */ ui::Color::green().v,
    /*.menu_location = */ app_location_t::RX,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = */ {'P', 'F', 'L', 'X'},
    /*.m4_app_offset = */ 0x00000000,
};
}