// Author: berkeozkir (Berke Özkır)

#include "ui.hpp"
#include "ui_fpv_detect.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::fpv_detect {
void initialize_app(ui::NavigationView& nav) {
    nav.push<FpvDetectView>();
}
}  // namespace ui::external_app::fpv_detect

extern "C" {

__attribute__((section(".external_app.app_fpv_detect.application_information"), used)) application_information_t _application_information_fpv_detect = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::fpv_detect::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "FPV DETECT",
    /*.bitmap_data = */
    {
        0x00,
        0x00,
        0xC0,
        0x03,
        0x20,
        0x04,
        0x10,
        0x08,
        0x48,
        0x12,
        0xA4,
        0x25,
        0x54,
        0x2A,
        0xA8,
        0x15,
        0x54,
        0x2A,
        0xA4,
        0x25,
        0x48,
        0x12,
        0x10,
        0x08,
        0x20,
        0x04,
        0xC0,
        0x03,
        0x00,
        0x00,
        0x00,
        0x00,
    },
    /*.icon_color = */ ui::Color::orange().v,
    /*.menu_location = */ app_location_t::RX,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_wfm_audio */ {'P', 'W', 'F', 'M'},
    /*.m4_app_offset = */ 0x00000000,
};
}
