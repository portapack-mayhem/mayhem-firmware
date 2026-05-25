/*

 * Copyright (C) 2026

 */

#include "meteor_standalone_ui.hpp"



#include "standalone_app.hpp"

#include "ui.hpp"

#include "ui_painter.hpp"



namespace meteor_standalone_ui {

namespace {



const standalone_application_api_t* bound_api{nullptr};



}  // namespace



void bind(const standalone_application_api_t& api) {

    bound_api = &api;

    _api = &api;

}



void clear_screen(const uint16_t w, const uint16_t h, const uint16_t color) {

    if (!bound_api)

        return;

    bound_api->fill_rectangle(0, 0, w, h, color);

}



void draw_text(const int x, const int y, const char* text, const uint16_t fg, const uint16_t bg) {

    if (!bound_api || !text || !bound_api->fixed_8x16_glyph_data)

        return;

    const ui::Font font{8, 16, bound_api->fixed_8x16_glyph_data, 0x20, 223};

    ui::Painter painter;

    ui::Style style{

        .font = font,

        .background = ui::Color{bg},

        .foreground = ui::Color{fg},

    };

    painter.draw_string({x, y}, style, text);

}



bool key_exit_to_menu(const uint8_t key) {

    if (key != kKeyLeft && key != kKeyBack)

        return false;

    if (bound_api)

        bound_api->exit_app();

    return true;

}



void draw_app_ui(

    const uint16_t screen_w,

    const uint16_t screen_h,

    const char* status,

    const char* hint1,

    const char* hint2,

    const char* action_hint) {

    clear_screen(screen_w, screen_h, 0);



    if (status)

        draw_text(0, 0, status, 0xFFFF, 0);

    if (hint1)

        draw_text(0, 20, hint1, 0xC618, 0);

    if (hint2)

        draw_text(0, 36, hint2, 0xC618, 0);



    const int footer_y = (screen_h >= 48) ? static_cast<int>(screen_h) - 48 : 240;

    draw_text(0, footer_y, "Left / Back: Receive menu", 0xAD55, 0);

    if (action_hint)

        draw_text(0, footer_y + 16, action_hint, 0x07E0, 0);

}



}  // namespace meteor_standalone_ui


