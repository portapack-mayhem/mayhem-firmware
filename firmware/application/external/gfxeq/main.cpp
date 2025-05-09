/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#include "ui_gfxeq.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::gfxeq {
void initialize_app(ui::NavigationView& nav) {
    nav.push<gfxEQView>();
}
}  // namespace ui::external_app::gfxeq

extern "C" {
__attribute__((section(".external_app.app_gfxeq.application_information"), used)) application_information_t _application_information_gfxeq = {
    (uint8_t*)0x00000000,
    ui::external_app::gfxeq::initialize_app,
    CURRENT_HEADER_VERSION,
    VERSION_MD5,
    "gfxEQ",
    {0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    ui::Color::green().v,
    app_location_t::RX,
    -1,
    {'P', 'W', 'F', 'M'},
    0x00000000,
};

}  // namespace ui::external_app::gfxeq
