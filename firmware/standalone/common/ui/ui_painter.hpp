/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __UI_PAINTER_H__
#define __UI_PAINTER_H__

#include "ui.hpp"
#include "ui_text.hpp"

#include <string_view>
#include <array>
#include <vector>

namespace ui {

struct Style {
    const Font& font;
    const Color background;
    const Color foreground;

    Style invert() const;
};

/* Sometimes mutation is just the more readable thing... */
struct MutableStyle {
    const Font* font;
    Color background;
    Color foreground;

    MutableStyle(const Style& s)
        : font{&s.font},
          background{s.background},
          foreground{s.foreground} {}

    void invert() {
        std::swap(background, foreground);
    }

    operator Style() const {
        return {
            .font = *font,
            .background = background,
            .foreground = foreground};
    }
};

class Widget;

class Painter {
   public:
    Painter() {};

    Painter(const Painter&) = delete;
    Painter(Painter&&) = delete;

    int draw_char(Point p, const Style& style, char c);

    int draw_string(Point p, const Style& style, std::string_view text);
    int draw_string(Point p, const Font& font, Color foreground, Color background, std::string_view text);

    void draw_bitmap(Point p, const Bitmap& bitmap, Color background, Color foreground);

    void draw_rectangle(Rect r, Color c);
    void fill_rectangle(Rect r, Color c);
    void fill_rectangle_unrolled8(Rect r, Color c);

    void paint_widget_tree(Widget* w);

    void draw_hline(Point p, int width, Color c);
    void draw_vline(Point p, int height, Color c);

    template <size_t N>
    void draw_pixels(
        const ui::Rect r,
        const std::array<ui::Color, N>& colors) {
        draw_pixels(r, colors.data(), colors.size());
    }

    void draw_pixels(
        const ui::Rect r,
        const std::vector<ui::Color>& colors) {
        draw_pixels(r, colors.data(), colors.size());
    }

    void draw_pixels(const ui::Rect r, const ui::Color* const colors, const size_t count);
    void draw_line(const ui::Point start, const ui::Point end, const ui::Color color);
    void draw_pixel(const ui::Point p, const ui::Color color);

   private:
    void paint_widget(Widget* w);
};

} /* namespace ui */

#endif /*__UI_PAINTER_H__*/
