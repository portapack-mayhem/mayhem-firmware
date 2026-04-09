#include "ui.hpp"
#include "ui_flex_tx.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::flex_tx {
void initialize_app(ui::NavigationView& nav) {
    nav.push<FlexTXView>();
}
}  // namespace ui::external_app::flex_tx

extern "C" {

__attribute__((section(".external_app.app_flex_tx.application_information"), used)) application_information_t _application_information_flex_tx = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::flex_tx::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "FLEX TX",
    /*.bitmap_data = */ {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0xFC,
        0x3F,
        0xFE,
        0x7F,
        0x02,
        0x40,
        0xBA,
        0x45,
        0x02,
        0x40,
        0xFE,
        0x7F,
        0xFE,
        0x7F,
        0x92,
        0x7C,
        0x92,
        0x7C,
        0xFC,
        0x3F,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    },
    /*.icon_color = */ ui::Color::cyan().v,
    /*.menu_location = */ app_location_t::TX,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_fsktx */ {'P', 'F', 'S', 'K'},
    /*.m4_app_offset = */ 0x00000000,
};
}
