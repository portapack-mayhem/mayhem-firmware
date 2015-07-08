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

#include "ui_painter.hpp"

namespace ui {

Style Style::invert() const {
	return {
		.font = font,
		.background = foreground,
		.foreground = background,
	};
}

size_t Painter::draw_char(const Point p, const Style& style, const char c) {
	const auto glyph = style.font.glyph(c);
	display_.draw_glyph(p, glyph, style.foreground, style.background);
	return glyph.advance().x;
}

size_t Painter::draw_string(Point p, const Style& style, const std::string text) {
	size_t width = 0;
	for(const auto c : text) {
		const auto glyph = style.font.glyph(c);
		display_.draw_glyph(p, glyph, style.foreground, style.background);
		const auto advance = glyph.advance();
		p += advance;
		width += advance.x;
	}
	return width;
}

void Painter::draw_hline(Point p, size_t width, const Color c) {
	display_.fill_rectangle({ p, { static_cast<Dim>(width), 1 } }, c);
}

void Painter::draw_vline(Point p, size_t height, const Color c) {
	display_.fill_rectangle({ p, { 1, static_cast<Dim>(height) } }, c);
}

void Painter::draw_rectangle(const Rect r, const Color c) {
	draw_hline(r.pos, r.size.w, c);
	draw_vline({ r.pos.x, static_cast<Coord>(r.pos.y + 1) }, r.size.h - 2, c);
	draw_vline({ static_cast<Coord>(r.pos.x + r.size.w - 1), static_cast<Coord>(r.pos.y + 1) }, r.size.h - 2, c);
	draw_hline({ r.pos.x, static_cast<Coord>(r.pos.y + r.size.h - 1) }, r.size.w, c);
}

void Painter::fill_rectangle(const Rect r, const Color c) {
	display_.fill_rectangle(r, c);
}

} /* namespace ui */
