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
}

extern "C" {

__attribute__((section(".external_app.app_cvs_spam.application_information"), used)) application_information_t _application_information_cvs_spam = {
   (uint8_t*)0x00000000,
   ui::external_app::cvs_spam::initialize_app,
   CURRENT_HEADER_VERSION,
   VERSION_MD5,
   "CVS Spam",
   {
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
       0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81
   },
   ui::Color::red().v,
   app_location_t::TX,
   {'P', 'R', 'E', 'P'},
   0x00000000
};

}