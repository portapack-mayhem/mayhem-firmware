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

#include <string>

namespace ui {

struct Style {
	const Font& font;
	const Color background;
	const Color foreground;

	Style invert() const;
};

class Widget;

class Painter {
public:
	Painter() { };

	Painter(const Painter&) = delete;
	Painter(Painter&&) = delete;

	int draw_char(const Point p, const Style& style, const char c);

	int draw_string(Point p, const Font& font, const Color foreground,
		const Color background, const std::string text);
	int draw_string(Point p, const Style& style, const std::string text);

	void draw_bitmap(const Point p, const Bitmap& bitmap, const Color background, const Color foreground);

	void draw_rectangle(const Rect r, const Color c);
	void fill_rectangle(const Rect r, const Color c);
	void fill_rectangle_unrolled8(const Rect r, const Color c);

	void paint_widget_tree(Widget* const w);
	
	void draw_hline(Point p, int width, const Color c);
	void draw_vline(Point p, int height, const Color c);
	
private:
	void paint_widget(Widget* const w);
};

} /* namespace ui */

#endif/*__UI_PAINTER_H__*/
