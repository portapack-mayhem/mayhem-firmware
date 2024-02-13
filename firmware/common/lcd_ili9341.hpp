/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#ifndef __LCD_ILI9341_H__
#define __LCD_ILI9341_H__

#include "ui.hpp"
#include "ui_text.hpp"
#include "file.hpp"

#include <cstdint>
#include <array>

namespace lcd {

class ILI9341 {
   public:
    constexpr ILI9341()
        : scroll_state{0, 0, height(), 0} {
    }

    ILI9341(const ILI9341&) = delete;
    ILI9341(ILI9341&&) = delete;
    void operator=(const ILI9341&) = delete;

    bool read_display_status();

    void init();
    void shutdown();

    void sleep();
    void wake();

    void fill_rectangle(ui::Rect r, const ui::Color c);
    void fill_rectangle_unrolled8(ui::Rect r, const ui::Color c);
    void draw_line(const ui::Point start, const ui::Point end, const ui::Color color);
    void fill_circle(
        const ui::Point center,
        const ui::Dim radius,
        const ui::Color foreground,
        const ui::Color background);

    void draw_pixel(const ui::Point p, const ui::Color color);
    void drawBMP(const ui::Point p, const uint8_t* bitmap, const bool transparency);
    bool drawBMP2(const ui::Point p, const std::filesystem::path& file);
    void render_line(const ui::Point p, const uint8_t count, const ui::Color* line_buffer);
    void render_box(const ui::Point p, const ui::Size s, const ui::Color* line_buffer);

    template <size_t N>
    void draw_pixels(
        const ui::Rect r,
        const std::array<ui::Color, N>& colors) {
        draw_pixels(r, colors.data(), colors.size());
    }

    template <size_t N>
    void read_pixels(
        const ui::Rect r,
        std::array<ui::ColorRGB888, N>& colors) {
        read_pixels(r, colors.data(), colors.size());
    }

    void draw_bitmap(
        const ui::Point p,
        const ui::Size size,
        const uint8_t* const data,
        const ui::Color foreground,
        const ui::Color background);

    void draw_glyph(
        const ui::Point p,
        const ui::Glyph& glyph,
        const ui::Color foreground,
        const ui::Color background);

    /*** Scrolling ***
     * Scrolling support is implemented in the ILI9341 driver. Basically a region
     * of the screen is set up to act as a circular buffer. The VSA (vertical scroll
     * address) is the line that defines the "start" of the circular buffer. In our
     * case, the driver is set up for "bottom-up" scrolling. In this mode, drawing
     * starts at the bottom of the scroll region and draws the buffer upward.
     * However, the whole display's address space is inverted (the screen is actually
     * upside down in the PortaPack) so this bottom-up drawing appears to be top-down.
     *
     * What this means is that the line pointed to by VSA will be drawn at the top
     * of the scroll region and the line at VSA - 1 (wrapped) will be the bottom.
     * Consider the following screen buffers and VSA pointers.
     *
     * Buffer:  Display:    Buffer:  Display:
     * VSA > A      A           A       C       NB: VSA points to the "Top"
     *       B      B           B       D           of the rendered output.
     *       C      C     VSA > C       A
     *       D      D           D       B
     */

    /* Sets the top and bottom lines of the scrolling region. */
    void scroll_set_area(const ui::Coord top_y, const ui::Coord bottom_y);
    void scroll_disable();

    /* Sets VSA. This an offset from top_y not a screen coordinate. */
    ui::Coord scroll_set_position(const ui::Coord position);

    /* Relative adjustment to VSA. Positive value will cause output
     * to scoll "down", negative values will cause the output to scroll "up". */
    ui::Coord scroll(const int32_t delta);

    /* Gets a screen coordinate from a scroll region offset. Values are wrapped
     * so the offset is wrapped within the bounds of the scroll region. The
     * specified offset is applied to VSA and then offset by the scroll region
     * top to produce a screen coordinate.
     * Consider the following screen buffers and VSA pointer. Note the offsets.
     *
     * VSA > A = +0          A = +2
     *       B = +1          B = +3   NB: As before, VSA is always the "top"
     *       C = +2    VSA > C = +0       and line at VSA - 1 will be the
     *       D = +3          D = +1       "bottom" or the max 'y' offset.
     */
    ui::Coord scroll_area_y(const ui::Coord y) const;

    constexpr ui::Dim width() const { return ui::screen_width; }
    constexpr ui::Dim height() const { return ui::screen_height; }
    constexpr ui::Rect screen_rect() const { return {0, 0, width(), height()}; }

    void draw_pixels(const ui::Rect r, const ui::Color* const colors, const size_t count);
    void read_pixels(const ui::Rect r, ui::ColorRGB888* const colors, const size_t count);

   private:
    struct scroll_t {
        ui::Coord top_area;
        ui::Coord bottom_area;
        ui::Dim height;
        ui::Coord current_position;
    };

    scroll_t scroll_state;
};

} /* namespace lcd */

#endif /*__LCD_ILI9341_H__*/
