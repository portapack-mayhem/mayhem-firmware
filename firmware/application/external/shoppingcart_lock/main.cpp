
// RocketGod's Shopping Cart Lock app
// https://betaskynet.com
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
    (uint8_t*)0x00000000,
    ui::external_app::shoppingcart_lock::initialize_app,
    CURRENT_HEADER_VERSION,
    VERSION_MD5,
    "Cart Lock",
    {
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
    ui::Color::red().v,
    app_location_t::UTILITIES,
    {'P', 'A', 'T', 'X'},
    0x00000000,
};
}