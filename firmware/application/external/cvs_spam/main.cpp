// CVS Spam app by RocketGod (@rocketgod-git) https://betaskynet.com
// Original .cu8 files by @jimilinuxguy https://github.com/jimilinuxguy/customer-assistance-buttons-sdr
// If you can read this, you're a nerd. :P
// Come join us at https://discord.gg/thepiratesreborn

#include "ui.hpp"
#include "cvs_spam.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::cvs_spam {
void initialize_app(NavigationView& nav) {
    nav.push<CVSSpamView>();
}
}  // namespace ui::external_app::cvs_spam

extern "C" {

__attribute__((section(".external_app.app_cvs_spam.application_information"), used)) application_information_t _application_information_cvs_spam = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::cvs_spam::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "CVS Spam",
    /*.bitmap_data = */ {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81},
    /*.icon_color = */ ui::Color::red().v,
    /*.menu_location = */ app_location_t::TX,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = portapack::spi_flash::image_tag_afsk_rx */ {'P', 'R', 'E', 'P'},
    /*.m4_app_offset = */ 0x00000000,  // will be filled at compile time
};
}