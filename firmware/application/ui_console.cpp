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

namespace ui {

void Console::write(const std::string message) {
	(void)message;
}

void Console::writeln(const std::string message) {
	write(message);
	crlf();
}

void Console::paint(Painter& painter) {
	(void)painter;
	/*
	if( visible() ) {
		const auto r = screen_rect();
		display.scroll_set_area(r.top(), r.bottom());
		display.scroll_set_position(0);
		painter.fill_rectangle(
			r,
			background
		);
	} else {
		display.scroll_disable();
	}
	*/
}

void Console::crlf() {
	const auto line_height = style().font.line_height();
	pos.x = 0;
	pos.y += line_height;
	const int32_t y_excess = pos.y + line_height - size().h;
	if( y_excess > 0 ) {
		display.scroll(-line_height);
		pos.y -= y_excess;

		const Rect dirty { 0, display.scroll_area_y(pos.y), size().w, line_height };
		display.fill_rectangle(dirty, background);
	}
}

} /* namespace ui */
