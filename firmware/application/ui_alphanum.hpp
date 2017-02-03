/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __ALPHANUM_H__
#define __ALPHANUM_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "ui_menu.hpp"
#include "ui_navigation.hpp"
#include "ui_font_fixed_8x16.hpp"

namespace ui {

class AlphanumView : public View {
public:
	std::function<void(char *)> on_changed { };

	AlphanumView(NavigationView& nav, char txt[], size_t max_length);
	
	AlphanumView(const AlphanumView&) = delete;
	AlphanumView(AlphanumView&&) = delete;
	AlphanumView& operator=(const AlphanumView&) = delete;
	AlphanumView& operator=(AlphanumView&&) = delete;


	void paint(Painter& painter) override;
	void focus() override;
	
	char * value();
	
	std::string title() const override { return "Text entry"; };

private:
	const char * const keys_upper = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ. !<";
	const char * const keys_lower = "0123456789abcdefghijklmnopqrstuvwxyz:=?<";
	
	size_t _max_length { };
	uint8_t txtidx { 0 };
	bool _lowercase = false;
	char txtinput[29] = { 0 };		// 28 chars max
	
	void char_add(const char c);
	void char_delete();
	void set_keys(const char * const key_list);
	void move_cursor();
	void on_button(Button& button);
	void update_text();
	
	Text text_input {
		{ 8, 0, 232, 16 }
	};

	std::array<Button, 40> buttons { };

	Button button_lowercase {
		{ 21 * 8, 270, 32, 24 },
		"UC"
	};
	
	NumberField raw_char {
		{ 16, 270 },
		3,
		{ 1, 255 },
		1,
		'0'
	};

	Button button_ok {
		{ 88, 270, 64, 24 },
		"OK"
	};
};

} /* namespace ui */

#endif/*__ALPHANUM_H__*/
