/*
 * Copyright (C) 2026 Ivan
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

/* HOST TEST SHIM for the Mayhem ui::Painter / ui::Style (never compiled into
 * firmware). It provides the exact surface ui_station_list.hpp uses:
 *
 *   - ui::Style{font, background, foreground} (same fields as
 *     firmware/common/ui_painter.hpp);
 *   - ui::Painter::fill_rectangle(Rect, Color) and draw_string(Point, Style,
 *     string), rasterizing with the REAL ui::font::fixed_8x16 glyph data via
 *     the real Font::glyph() (ui_text.cpp) and the same bit order as
 *     ILI9341::draw_bitmap (LSB-first, row-major);
 *   - a HostFramebuffer target (RGB565 .v values kept raw) plus a log of
 *     every fill/draw_string call, so tests can assert on both pixels and
 *     drawn strings.
 */

#ifndef __WSPR_UI_SHIM_UI_PAINTER_H__
#define __WSPR_UI_SHIM_UI_PAINTER_H__

#include "ui.hpp"
#include "ui_text.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ui {

struct Style {
    const Font& font;
    Color background;
    Color foreground;
};

struct HostFramebuffer {
    int width{0};
    int height{0};
    std::vector<uint16_t> pixels{};  // RGB565, row-major, width*height

    struct StringDraw {
        Point p;
        Style style;
        std::string text;
    };
    std::vector<StringDraw> strings;
    struct Fill {
        Rect r;
        Color color;
    };
    std::vector<Fill> fills;

    void reset(const int w, const int h) {
        width = w;
        height = h;
        pixels.assign((size_t)w * h, 0);
        strings.clear();
        fills.clear();
    }

    uint16_t at(const int x, const int y) const {
        if (x < 0 || y < 0 || x >= width || y >= height)
            return 0;
        return pixels[(size_t)y * width + x];
    }
};

class Painter {
   public:
    void target(HostFramebuffer* fb) { fb_ = fb; }

    /* Optional screen-space translation applied to every fill/draw, mirroring
     * the framework drawing a pushed application at its real origin (physical
     * y+16 below the 16-px system status strip). Default {0,0} leaves the
     * existing single-widget tests unchanged. The draw LOG stores the
     * translated (physical) coordinates, so assertions inspect what the screen
     * actually shows. */
    void set_origin(const Point origin) { origin_ = origin; }
    Point origin() const { return origin_; }

    void fill_rectangle(const Rect rect_in, const Color color) {
        if (!fb_)
            return;
        const Rect rect{rect_in.left() + origin_.x(),
                        rect_in.top() + origin_.y(), rect_in.width(),
                        rect_in.height()};
        fb_->fills.push_back({rect, color});
        for (int y = rect.top(); y < rect.bottom(); y++)
            for (int x = rect.left(); x < rect.right(); x++)
                if (x >= 0 && y >= 0 && x < fb_->width && y < fb_->height)
                    fb_->pixels[(size_t)y * fb_->width + x] = color.v;
    }

    void draw_string(const Point p, const Style& style, const std::string_view text) {
        if (!fb_)
            return;
        Point q = {p.x() + origin_.x(), p.y() + origin_.y()};
        /* Log the translated (physical) position so assertions see the same
         * coordinates the screen actually received. */
        fb_->strings.push_back({q, style, std::string(text)});
        for (const char c : text) {
            const auto glyph = style.font.glyph(c);
            const uint8_t* data = glyph.pixels();
            const int w = glyph.w();
            const int h = glyph.h();
            /* Same pixel order as ILI9341::draw_bitmap: row-major,
             * bit i of the byte is pixel (i & 7). */
            for (int i = 0; i < w * h; i++) {
                const bool on = data[i >> 3] & (1U << (i & 7));
                const int x = q.x() + (i % w);
                const int y = q.y() + (i / w);
                if (x >= 0 && y >= 0 && x < fb_->width && y < fb_->height)
                    fb_->pixels[(size_t)y * fb_->width + x] =
                        on ? style.foreground.v : style.background.v;
            }
            q += glyph.advance();
        }
    }

   private:
    HostFramebuffer* fb_{nullptr};
    Point origin_{0, 0};
};

} /* namespace ui */

#endif /*__WSPR_UI_SHIM_UI_PAINTER_H__*/
