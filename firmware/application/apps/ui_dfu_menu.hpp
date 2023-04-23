/*
 * Copyright (C) 2023 Bernd Herzog
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

#ifndef __UI_DFU_MENU_H__
#define __UI_DFU_MENU_H__

#include <cstdint>

#include "ui_widget.hpp"
#include "event_m0.hpp"
#include "debug.hpp"
#include "string_format.hpp"

#define LINE_HEIGHT 16
#define CHARACTER_WIDTH 8

namespace ui {
class NavigationView;

class DfuMenu : public View {
public:
	DfuMenu(NavigationView& nav);
	~DfuMenu() = default;

	void paint(Painter& painter) override;

private:
	NavigationView& nav_;
	
	Text text_head {{ 6 * CHARACTER_WIDTH, 3 * LINE_HEIGHT, 11 * CHARACTER_WIDTH, 1 * LINE_HEIGHT }, "Performance"};

	Labels labels {
		{ { 6 * CHARACTER_WIDTH, 5 * LINE_HEIGHT }, "M0 heap:", Color::dark_cyan() },
		{ { 6 * CHARACTER_WIDTH, 6 * LINE_HEIGHT }, "M0 stack:", Color::dark_cyan() },
		{ { 6 * CHARACTER_WIDTH, 7 * LINE_HEIGHT }, "M0 cpu %:", Color::dark_cyan() },
		{ { 6 * CHARACTER_WIDTH, 8 * LINE_HEIGHT }, "M4 heap:", Color::dark_cyan() },
		{ { 6 * CHARACTER_WIDTH, 9 * LINE_HEIGHT }, "M4 stack:", Color::dark_cyan() },
		{ { 6 * CHARACTER_WIDTH,10 * LINE_HEIGHT }, "M4 cpu %:", Color::dark_cyan() },
		{ { 6 * CHARACTER_WIDTH,11 * LINE_HEIGHT }, "uptime:", Color::dark_cyan() }
	};

	Text text_info_line_1 {{ 15 * CHARACTER_WIDTH, 5 * LINE_HEIGHT, 5 * CHARACTER_WIDTH, 1 * LINE_HEIGHT }, ""};
	Text text_info_line_2 {{ 15 * CHARACTER_WIDTH, 6 * LINE_HEIGHT, 5 * CHARACTER_WIDTH, 1 * LINE_HEIGHT }, ""};
	Text text_info_line_3 {{ 15 * CHARACTER_WIDTH, 7 * LINE_HEIGHT, 5 * CHARACTER_WIDTH, 1 * LINE_HEIGHT }, ""};
	Text text_info_line_4 {{ 15 * CHARACTER_WIDTH, 8 * LINE_HEIGHT, 5 * CHARACTER_WIDTH, 1 * LINE_HEIGHT }, ""};
	Text text_info_line_5 {{ 15 * CHARACTER_WIDTH, 9 * LINE_HEIGHT, 5 * CHARACTER_WIDTH, 1 * LINE_HEIGHT }, ""};
	Text text_info_line_6 {{ 15 * CHARACTER_WIDTH,10 * LINE_HEIGHT, 5 * CHARACTER_WIDTH, 1 * LINE_HEIGHT }, ""};
	Text text_info_line_7 {{ 15 * CHARACTER_WIDTH,11 * LINE_HEIGHT, 5 * CHARACTER_WIDTH, 1 * LINE_HEIGHT }, ""};
};

} /* namespace ui */

#endif/*__UI_DFU_MENU_H__*/
