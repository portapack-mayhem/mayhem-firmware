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

#include "ui_handwrite.hpp"

#include "ch.h"

#include "ff.h"
#include "unistroke.hpp"
#include "portapack.hpp"
#include "event_m0.hpp"
#include "string_format.hpp"
#include "hackrf_hal.hpp"
#include "portapack_shared_memory.hpp"
#include "time.hpp"

#include <cstring>

using namespace portapack;

namespace ui {

HandWriteView::HandWriteView(
	NavigationView& nav,
	char txt[],
	uint8_t max_len
) {
	_max_len = max_len;
	_lowercase = false;
	
	txtidx = 0;
	//memcpy(txtinput, txt, max_len+1);
	
	add_child(&text_input);

	const auto button_fn = [this](Button& button) {
		this->on_button(button);
	};

	size_t n = 0;
	for(auto& button : num_buttons) {
		add_child(&button);
		button.on_select = button_fn;
		button.set_parent_rect({
			static_cast<Coord>(n * 24),
			static_cast<Coord>(240),
			24, 20
		});
		const std::string label {
			(char)(n + 0x30)
		};
		button.set_text(label);
		button.id = n;
		n++;
	}
	//set_uppercase();
	
	/*add_child(&button_lowercase);
	button_lowercase.on_select = [this, &nav, txt, max_len](Button&) {
		if (_lowercase == true) {
			_lowercase = false;
			button_lowercase.set_text("UC");
			set_uppercase();
		} else {
			_lowercase = true;
			button_lowercase.set_text("LC");
			set_lowercase();
		}
	};*/

	add_child(&text_debug_x);
	add_child(&text_debug_y);
	add_child(&button_done);
	button_done.on_select = [this, &nav, txt, max_len](Button&) {
		//memcpy(txt, txtinput, max_len+1);
		//on_changed(this->value());
		nav.pop();
	};

	//update_text();
}

bool HandWriteView::on_touch(const TouchEvent event) {
	if (event.type == ui::TouchEvent::Type::Start) {
		move_index = 0;
		move_wait = 4;
		tracing = true;
	}
	if (event.type == ui::TouchEvent::Type::End) {
		tracing = false;
		guess_letter();
	}
	if (event.type == ui::TouchEvent::Type::Move) {
		if (tracing) {
			current_pos = event.point;
		}
	}
	return true;
}

void HandWriteView::guess_letter() {
	uint8_t symbol, match, count, stroke_idx, stroke_data;
	Condition cond;
	Direction dir;
	bool matched;
	
	// Clear drawing zone
	display.fill_rectangle(
		{{0, 16}, {240, 240}},
		Color::black()
	);
	
	// Letter guessing
	if (move_index) {
		for (symbol = 0; symbol < handwriting_unistroke.letter_count; symbol++) {
			count = handwriting_unistroke.letter[symbol].count;
			matched = false;
			if (count) {
				// We have a count match to do
				if ((count == 1) && (move_index == 1)) matched = true;
				if ((count == 2) && (move_index == 2)) matched = true;
				if ((count == 3) && (move_index > 2)) matched = true;
			} else {
				matched = true;
			}
			if (matched) {
				for (match = 0; match < 3; match++) {
					cond = handwriting_unistroke.letter[symbol].match[match].cond;
					dir = handwriting_unistroke.letter[symbol].match[match].dir;
					if ((cond != cond_empty) && (dir != dir_empty)) {
						if (cond == last) {
							if (move_index)
								stroke_idx = move_index - 1;
							else
								stroke_idx = 0;
						} else if (cond == stroke_a)
							stroke_idx = 0;
						else if (cond == stroke_b)
							stroke_idx = 1;
						else if (cond == stroke_c)
							stroke_idx = 2;
						else
							stroke_idx = 3;
						stroke_data = move_list[stroke_idx];
						if ((dir & 0xF0) == 0xF0) {
							if ((dir & 0x0F) != (stroke_data & 0x0F)) break;
						} else if ((dir & 0x0F) == 0x0F) {
							if ((dir & 0xF0) != (stroke_data & 0xF0)) break;
						} else {
							if (dir != stroke_data) break;
						}
					}
				}
				if (match == 3)
					break;
				else
					matched = false;
			}
		}
		if (matched) {
			if (symbol)
				txtinput[txtidx++] = 'A' + symbol - 1;
			else
				txtinput[--txtidx] = 0;		// Erase
		}
	} else {
		txtinput[txtidx++] = ' ';
	}
	update_text();
	move_index = 0;
}

void HandWriteView::add_stroke(uint8_t dir) {
	if (move_index < 8) {
		move_list[move_index] = dir;
		move_index++;
	} else {
		guess_letter();
	}
}

void HandWriteView::sample_pen() {
	int16_t diff_x, diff_y;
	uint8_t dir, dir_ud, dir_lr;
	
	// Blink cursor
	if (!(sample_skip & 15)) {
		Point cursor_pos;
		
		cursor_pos.x = text_input.screen_rect().pos.x + (txtidx * 8);
		cursor_pos.y = text_input.screen_rect().pos.y + 17;
		
		if (cursor) {
			display.fill_rectangle(
				{{text_input.screen_rect().pos.x, cursor_pos.y}, {text_input.screen_rect().size.w, 4}},
				Color::black()
			);
		} else {
			display.fill_rectangle(
				{cursor_pos, {8, 4}},
				Color::white()
			);
		}
		cursor = !cursor;	
	}
	
	if (!(sample_skip & 1)) {
		if (tracing) {
			if (move_wait) {
				move_wait--;	// ~133ms delay
			} else {
				diff_x = current_pos.x - last_pos.x;
				diff_y = current_pos.y - last_pos.y;
				
				text_debug_x.set(to_string_dec_int(diff_x));
				text_debug_y.set(to_string_dec_int(diff_y));
				
				if (current_pos.y <= 240) {
					display.fill_rectangle(
						{{current_pos.x, current_pos.y}, {4, 4}},
						Color::grey()
					);
				}
				
				dir = 0;
				if (abs(diff_x) > 7) {
					if (diff_x > 0)
						dir |= 0x01;	// R
				} else {
					dir |= 0x02;		// ?
				}
				if (abs(diff_y) > 7) {
					if (diff_y > 0)
						dir |= 0x10;	// D
				} else {
					dir |= 0x20;		// ?
				}
				
				if ((dir & 0x11) == (dir_prev & 0x11))
					dir_cnt++;
				else
					dir_cnt = 0;
				dir_prev = dir;

				// text_debug_d.set(to_string_dec_uint(dir));

				if (dir_cnt > 1) {
					dir_cnt = 0;
					if (move_index) {
						if ((move_list[move_index - 1] != dir) && (dir != 0x22)) {
							dir_ud = (dir & 0xF0);
							dir_lr = (dir & 0x0F);
							if (dir_ud == 0x20) {
								if ((move_list[move_index - 1] & 0x0F) != dir_lr) add_stroke(dir);
							} else if (dir_lr == 0x02) {
								if ((move_list[move_index - 1] & 0xF0) != dir_ud) add_stroke(dir);
							} else {
								// Replacement ?
								if (((move_list[move_index - 1] & 0xF0) == 0x20) && (dir_ud != 0x20)) {
									if ((move_list[move_index - 1] & 0x0F) == dir_lr) {
										move_list[move_index - 1] = dir;
									} else if ((dir & 0x0F) == 0x02) {
										// Merge
										move_list[move_index - 1] = dir_ud | (move_list[move_index - 1] & 0x0F);
									} else {
										add_stroke(dir);
									}
								} else if (((move_list[move_index - 1] & 0x0F) == 0x02) && (dir_lr != 0x02)) {
									if ((move_list[move_index - 1] & 0xF0) == dir_ud) {
										move_list[move_index - 1] = dir;
									} else if (dir_ud == 0x20) {
										// Merge
										move_list[move_index - 1] = dir_lr | (move_list[move_index - 1] & 0xF0);
									} else {
										add_stroke(dir);
									}
								} else {
									add_stroke(dir);
								}
							}
						}
					} else {
						if (dir != 0x22) add_stroke(dir);
					}
				}
			}
		
			last_pos = current_pos;
		}
	}
	
	sample_skip++;
}

void HandWriteView::on_show() {
	// Use screen refresh rate as sampling frequency
	EventDispatcher::message_map().unregister_handler(Message::ID::DisplayFrameSync);
	EventDispatcher::message_map().register_handler(Message::ID::DisplayFrameSync,
		[this](const Message* const) {
			sample_pen();
		}
	);
}

char * HandWriteView::value() {
	return txtinput;
}

void HandWriteView::on_button(Button& button) {
	char_add(button.id + 0x30);
	update_text();
}

void HandWriteView::char_add(const char c) {
	if (txtidx < _max_len) {
		txtinput[txtidx] = c;
		txtidx++;
	}
}

void HandWriteView::update_text() {
	text_input.set(txtinput);
}

}
