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

#ifndef __UI_CONSOLE_H__
#define __UI_CONSOLE_H__

#include "ui.hpp"
#include "ui_painter.hpp"
#include "ui_widget.hpp"

#include "lcd_ili9341.hpp"

#include <string>

namespace ui {

class Console : public Widget {
public:
	constexpr Console(
		const Rect parent_rect,
		lcd::ILI9341& display
	) : Widget { parent_rect },
		display(display)
	{
	}

	void write(const std::string message);
	void writeln(const std::string message);

	void paint(Painter& painter) override;

private:
	static constexpr Color background { Color::black() };
	static constexpr Color foreground { Color::white() };

	Point pos { 0, 0 };
	lcd::ILI9341& display;

	void crlf();
};

} /* namespace ui */

#endif/*__UI_CONSOLE_H__*/
