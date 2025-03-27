#include "ui_rf3d.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::rf3d {
void initialize_app(ui::NavigationView& nav) {
    nav.push<RF3DView>();
}
}

extern "C" {
__attribute__((section(".external_app.app_3drf.application_information"), used)) application_information_t _application_information_3drf = {
    (uint8_t*)0x00000000,
    ui::external_app::rf3d::initialize_app,
    CURRENT_HEADER_VERSION,
    VERSION_MD5,
    "RF3D",
    {
        0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    },
    /*.icon_color = */ ui::Color::green().v,
    /*.menu_location = */ app_location_t::RX,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_wfm_audio */ {'P', 'N', 'F', 'M'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}