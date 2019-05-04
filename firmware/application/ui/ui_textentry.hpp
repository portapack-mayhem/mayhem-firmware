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

#ifndef __UI_TEXTENTRY_H__
#define __UI_TEXTENTRY_H__

#include "ui.hpp"
#include "ui_navigation.hpp"

namespace ui {

class TextEntryView : public View {
public:
	std::function<void(std::string&)> on_changed { };
	
	void focus() override;
	std::string title() const override { return "Text entry"; };
	
protected:
	TextEntryView(NavigationView& nav, std::string& str, size_t max_length);
	
	TextEntryView(const TextEntryView&) = delete;
	TextEntryView(TextEntryView&&) = delete;
	TextEntryView& operator=(const TextEntryView&) = delete;
	TextEntryView& operator=(TextEntryView&&) = delete;

	void char_add(const char c);
	void char_delete();
	void draw_cursor();
	void update_text();
	
	std::string& _str;
	size_t _max_length;
	uint32_t cursor_pos { 0 };
	
	Text text_input {
		{ 0, 0, 240, 16 }
	};
	
	Button button_ok {
		{ 10 * 8, 33 * 8, 9 * 8, 32 },
		"OK"
	};
};

void text_prompt(NavigationView& nav, std::string& str, size_t max_length, const std::function<void(std::string&)> on_done = nullptr);

} /* namespace ui */

#endif/*__UI_TEXTENTRY_H__*/
