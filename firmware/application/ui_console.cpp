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

#include "ui_console.hpp"

#include "portapack.hpp"
using namespace portapack;

namespace ui {

void Console::clear() {
	display.fill_rectangle(
		screen_rect(),
		Color::black()
	);
	pos = { 0, 0 };
	display.scroll_set_position(0);
}

void Console::write(const std::string& message) {
	const Style& s = style();
	const Font& font = s.font;
	const auto rect = screen_rect();
	for(const auto c : message) {
		if( c == '\n' ) {
			crlf();
		} else {
			const auto glyph = font.glyph(c);
			const auto advance = glyph.advance();
			if( (pos.x() + advance.x()) > rect.width() ) {
				crlf();
			}
			const Point pos_glyph {
				rect.pos.x() + pos.x(),
				display.scroll_area_y(pos.y())
			};
			display.draw_glyph(pos_glyph, glyph, s.foreground, s.background);
			pos += { advance.x(), 0 };
		}
	}
}

void Console::writeln(const std::string& message) {
	write(message);
	crlf();
}

void Console::paint(Painter& painter) {
	// Do nothing.
	(void)painter;
}

void Console::on_show() {
	const auto screen_r = screen_rect();
	display.scroll_set_area(screen_r.top(), screen_r.bottom());

	clear();
}

void Console::on_hide() {
	/* TODO: Clear region to eliminate brief flash of content at un-shifted
	 * position?
	 */
	display.scroll_disable();
}

void Console::crlf() {
	const Style& s = style();
	const auto sr = screen_rect();
	const auto line_height = s.font.line_height();
	pos = { 0, pos.y() + line_height };
	const int32_t y_excess = pos.y() + line_height - sr.height();
	if( y_excess > 0 ) {
		display.scroll(-y_excess);
		pos = { pos.x(), pos.y() - y_excess };

		const Rect dirty { sr.left(), display.scroll_area_y(pos.y()), sr.width(), line_height };
		display.fill_rectangle(dirty, s.background);
	}
}

} /* namespace ui */
