// CVS Spam app by RocketGod (@rocketgod-git) https://betaskynet.com
// Original .cu8 files by @jimilinuxguy https://github.com/jimilinuxguy/customer-assistance-buttons-sdr
// If you can read this, you're a nerd. :P
// Come join us at https://discord.gg/thepiratesreborn

#include "ui.hpp"
#include "shoppingcart_lock.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::shoppingcart_lock {
void initialize_app(NavigationView& nav) {
    baseband::run_image(portapack::spi_flash::image_tag_audio_tx);
    nav.push<ShoppingCartLock>();
}
}  // namespace ui::external_app::shoppingcart_lock

extern "C" {

__attribute__((section(".external_app.app_shoppingcart_lock.application_information"), used)) application_information_t _application_information_shoppingcart_lock = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::shoppingcart_lock::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "Cart Lock",
    /*.bitmap_data = */ {
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
        0x7E,
        0x7E,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x7E,
        0x81,
        0x81,
        0x7E,
        0x00,
        0x00,
        0x00,
        0x7E,
        0x81,
        0x81,
        0x81,
        0x81,
        0x7E,
        0x00,
    },
    /*.icon_color = */ ui::Color::red().v,
    /*.menu_location = */ app_location_t::UTILITIES,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_audio_tx */ {'P', 'A', 'T', 'X'},
    /*.m4_app_offset = */ 0x00000000,
};
}