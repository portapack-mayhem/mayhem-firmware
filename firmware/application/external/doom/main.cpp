/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

 #include "ui_doom.hpp"
 #include "ui_navigation.hpp"
 #include "external_app.hpp"
 
 namespace ui::external_app::doom {
 void initialize_app(ui::NavigationView& nav) {
     nav.push<DoomView>();
 }
 }  // namespace ui::external_app::doom
 
 extern "C" {
 
 __attribute__((section(".external_app.app_doom.application_information"), used)) application_information_t _application_information_doom = {
     (uint8_t*)0x00000000,
     ui::external_app::doom::initialize_app,
     CURRENT_HEADER_VERSION,
     VERSION_MD5,
     "Doom",
     {
        0x00,
        0x00,
        0x00,
        0x00,
        0x27,
        0xDA,
        0x59,
        0xAD,
        0x89,
        0xA8,
        0x89,
        0xA8,
        0x89,
        0xA8,
        0x89,
        0x88,
        0x89,
        0x88,
        0x89,
        0x88,
        0x59,
        0x8D,
        0x29,
        0x8A,
        0x05,
        0x80,
        0x03,
        0x80,
        0x00,
        0x00,
        0x00,
        0x00,
     },
     ui::Color::red().v,
     app_location_t::GAMES,
     -1,
     {0, 0, 0, 0},
     0x00000000,
 };
 }