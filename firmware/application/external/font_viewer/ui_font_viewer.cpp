/*
 * Copyright (C) 2023 Mark Thompson
 * copyleft Whiterose of the Dark Army
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "ui_font_viewer.hpp"

#include "ui_font_fixed_8x16.hpp"
#include "ui_painter.hpp"

#include "portapack.hpp"
using namespace portapack;
using namespace ui;

namespace ui::external_app::font_viewer {

/* DebugFontsView *******************************************************/

uint16_t DebugFontsView::display_font(Painter& painter, uint16_t y_offset, const Style* font_style, std::string_view font_name, bool is_big_font) {
    auto char_width{font_style->font.char_width()};
    auto char_height{font_style->font.line_height()};
    auto cpl{((screen_width / char_width) - 6) & 0xF8};  // Display a multiple of 8 characters per line
    uint16_t line_pos{y_offset};

    painter.draw_string({0, y_offset}, *font_style, font_name);

    // Displaying ASCII+extended characters from 0x20 to 0xFF
    for (uint8_t c = 0; c <= 0xDF; c++) {
        line_pos = y_offset + ((c / cpl) + 2) * char_height;

        if ((c % cpl) == 0)
            painter.draw_string({0, line_pos}, *font_style, "Ox" + to_string_hex(c + 0x20, 2));

        Style highlight_style_big = *Theme::getInstance()->fg_red;
        Style highlight_style_small = *Theme::getInstance()->bg_important_small;
        if (c == field_cursor.value()) {
            painter.draw_char({((c % cpl) + 5) * char_width, line_pos},
                              is_big_font ? highlight_style_big : highlight_style_small,
                              (char)(c + 0x20));
        } else {
            painter.draw_char({((c % cpl) + 5) * char_width, line_pos}, *font_style, (char)(c + 0x20));
        }
    }

    return line_pos + char_height;
}

void DebugFontsView::paint(Painter& painter) {
    int16_t line_pos;

    line_pos = display_font(painter, 32, Theme::getInstance()->bg_darkest, "Fixed 8x16", true);
    display_font(painter, line_pos + 16, Theme::getInstance()->bg_darkest_small, "Fixed 5x8", false);
}

void DebugFontsView::update_address_text() {
    uint8_t ascii_value = field_cursor.value() + 0x20;
    text_address.set("0x" + to_string_hex(ascii_value, 2));
}

DebugFontsView::DebugFontsView(NavigationView& nav)
    : nav_{nav} {
    add_children({&field_cursor,
                  &text_address});
    set_focusable(true);

    field_cursor.on_change = [&](int32_t) {
        update_address_text();
        set_dirty();
    };
}

void DebugFontsView::focus() {
    field_cursor.focus();
}

} /* namespace ui::external_app::font_viewer */
