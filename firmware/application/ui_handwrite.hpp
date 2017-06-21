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

#ifndef __UNISTROKE_H__
#define __UNISTROKE_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "ui_textentry.hpp"
#include "unistroke.hpp"

namespace ui {

class HandWriteView : public TextEntryView {
public:
	HandWriteView(NavigationView& nav, std::string * str, size_t max_length);

	HandWriteView(const HandWriteView&) = delete;
	HandWriteView(HandWriteView&&) = delete;
	HandWriteView& operator=(const HandWriteView&) = delete;
	HandWriteView& operator=(HandWriteView&&) = delete;
	
	void paint(Painter& painter) override;
	void on_show() override;
	bool on_touch(const TouchEvent event) override;
	
private:
	const char special_chars[5] = { '\'', '.', '?', '!', '=' }; 
	
	const HandWriting * handwriting { };
	Painter * _painter { };
	uint8_t dir_cnt { 0 };
	uint8_t dir_prev { 0 };
	uint8_t flash_timer { 0 };
	bool tracing { false };
	uint8_t stroke_index { 0 };
	uint8_t sample_skip { 0 }, move_wait { 0 };
	uint8_t stroke_list[8];
	Point start_pos { }, current_pos { }, last_pos { };
	bool _lowercase = false;
	
	void sample_pen();
	void add_stroke(uint8_t dir);
	void guess_letter();
	void clear_zone(const Color color, const bool flash);
	void on_button(Button& button);
	
	std::array<Button, 10> num_buttons { };
	std::array<Button, 5> special_buttons { };

	Button button_case {
		{ 8, 270, 32, 28 },
		"UC"
	};

	Button button_ok {
		{ 190, 270, 40, 28 },
		"OK"
	};
	
	MessageHandlerRegistration message_handler_sample {
		Message::ID::DisplayFrameSync,
		[this](const Message* const) {
			this->sample_pen();
		}
	};
};

} /* namespace ui */

#endif/*__UNISTROKE_H__*/
