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

#include <cstdint>
#include <array>

namespace lcd {

class ILI9341 {
public:
	constexpr ILI9341(
	) : scroll_state { 0, 0, height(), 0 }
	{
	}

	ILI9341(const ILI9341&) = delete;
	ILI9341(ILI9341&&) = delete;
	void operator=(const ILI9341&) = delete;

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
		const ui::Color background
	);

	void draw_pixel(const ui::Point p, const ui::Color color);
	void drawBMP(const ui::Point p, const uint8_t * bitmap, const bool transparency);
	void render_line(const ui::Point p, const uint8_t count, const ui::Color* line_buffer);
	void render_box(const ui::Point p, const ui::Size s, const ui::Color* line_buffer);
	
	template<size_t N>
	void draw_pixels(
		const ui::Rect r,
		const std::array<ui::Color, N>& colors
	) {
		draw_pixels(r, colors.data(), colors.size());
	}

	template<size_t N>
	void read_pixels(
		const ui::Rect r,
		std::array<ui::ColorRGB888, N>& colors
	) {
		read_pixels(r, colors.data(), colors.size());
	}

	void draw_bitmap(
		const ui::Point p,
		const ui::Size size,
		const uint8_t* const data,
		const ui::Color foreground,
		const ui::Color background
	);

	void draw_glyph(
		const ui::Point p,
		const ui::Glyph& glyph,
		const ui::Color foreground,
		const ui::Color background
	);

	void scroll_set_area(const ui::Coord top_y, const ui::Coord bottom_y);
	ui::Coord scroll_set_position(const ui::Coord position);
	ui::Coord scroll(const int32_t delta);
	ui::Coord scroll_area_y(const ui::Coord y) const;
	void scroll_disable();

	constexpr ui::Dim width() const { return 240; }
	constexpr ui::Dim height() const { return 320; }
	constexpr ui::Rect screen_rect() const { return { 0, 0, width(), height() }; }

private:
	struct scroll_t {
		ui::Coord top_area;
		ui::Coord bottom_area;
		ui::Dim height;
		ui::Coord current_position;
	};
	
	scroll_t scroll_state;

	void draw_pixels(const ui::Rect r, const ui::Color* const colors, const size_t count);
	void read_pixels(const ui::Rect r, ui::ColorRGB888* const colors, const size_t count);
};

} /* namespace lcd */

#endif/*__LCD_ILI9341_H__*/
