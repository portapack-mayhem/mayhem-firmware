/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

bool HandWriteView::MM(uint8_t idx, char cmp) {
	if (idx < move_index) {
		if ((cmp == 'U') && ((move_list[idx] & 0xF0) == 0x00)) return true;
		if ((cmp == 'D') && ((move_list[idx] & 0xF0) == 0x10)) return true;
		if ((cmp == 'L') && ((move_list[idx] & 0x0F) == 0x00)) return true;
		if ((cmp == 'R') && ((move_list[idx] & 0x0F) == 0x01)) return true;
	}
	return false;
}

bool HandWriteView::MM(uint8_t idx, char cmpud, char cmplr) {
	if (idx < move_index) {
		if (cmpud == 'U') cmpud = 0;
		if (cmpud == 'D') cmpud = 1;
		if (cmpud == '?') cmpud = 2;
		if (cmplr == 'L') cmplr = 0;
		if (cmplr == 'R') cmplr = 1;
		if (cmplr == '?') cmplr = 2;
		if (((move_list[idx] >> 4) == cmpud) && ((move_list[idx] & 0x0F) == cmplr)) return true;
	}
	return false;
}

bool HandWriteView::MI(uint8_t idx) {
	if (move_index - 1 < idx)
		return true;
	else
		return false;
}

bool HandWriteView::MLAST(char cmp) {
	if ((cmp == 'U') && ((move_list[move_index - 1] & 0xF0) == 0x00)) return true;
	if ((cmp == 'D') && ((move_list[move_index - 1] & 0xF0) == 0x10)) return true;
	if ((cmp == 'L') && ((move_list[move_index - 1] & 0x0F) == 0x00)) return true;
	if ((cmp == 'R') && ((move_list[move_index - 1] & 0x0F) == 0x01)) return true;
	return false;
}

bool HandWriteView::on_touch(const TouchEvent event) {
	char guess;
	
	if (event.type == ui::TouchEvent::Type::Start) {
		move_index = 0;
		move_wait = 4;
		tracing = true;
	}
	if (event.type == ui::TouchEvent::Type::End) {
		tracing = false;
		
		display.fill_rectangle(
			{{0, 16}, {240, 230}},
			Color::black()
		);
		
		// Letter guessing
		guess = ' ';
		
		if (MM(0, 'U')) {
			if (MM(0, 'U', '?')) {
				if (MI(1))
					guess = 'A';
				else
					guess = 'F';
			} else if (MM(0, 'U', 'R')) {
				if (MI(1)) {
					guess = 'K';
				} else {
					if (MM(1, 'L')) {
						if (txtidx > 0) {
							txtinput[--txtidx] = 0;	// Erase
							guess = '!';
						}
					} else {
						guess = 'N';
					}
				}
			} else if (MM(0, 'U', 'L')) {
				if (MM(1, 'U', 'R')) guess = 'C';
				if (MM(1, 'D', 'L')) guess = 'M';
			}
		} else if (MM(0, 'D')) {
			if (MM(0, 'D', 'R') || MM(1, 'R'))
				guess = 'P';
			else
				guess = 'Q';
			if (MM(0, 'D', '?')) {
				if (MI(1)) {
					guess = 'I';
				} else {
					if (MM(1, 'R') && MI(2)) {
						guess = 'L';
					} else if (MM(1, 'L')) {
						if (MI(2)) guess = 'J';
					} else if (MM(1, 'U', 'R')) {
						if (MM(2, 'D')) guess = 'W';
					}
				}
			}
			if (MM(0, 'D', 'R')) {
				if (MI(1)) guess = 'R';
				if (MM(1, 'U', 'R') && MI(2)) guess = 'V';
				if (MM(1, 'D', 'L')) guess = 'B';
			} else if (MM(0, 'D', 'L')) {
				if (MI(1)) guess = 'Y';
				if (MM(1, 'U', 'L') && MI(2)) guess = 'U';
				if (MM(1, 'D', 'R')) guess = 'D';
			}
		}
		
		if (MM(0, 'L')) {
			if (!MI(2) && (MLAST('U') || MLAST('L'))) guess = 'O';
			if (MM(0, '?', 'L')) {
				if (MI(1))
					guess = 'E';
				else
					if (MM(1, 'R')) guess = 'S';
			}
		} else if (MM(0, 'R')) {
			if (!MI(2) && (MLAST('U') || MLAST('R'))) guess = 'X';
			if (MM(0, '?', 'R')) {
				if (MM(1, 'U') && MI(2)) {
					guess = 'G';
				} else if (MM(1, 'D', '?') && MI(2)) {
					guess = 'H';
				} else if (MM(1, 'L')) {
					guess = 'Z';
				} else if (MI(1)) {
					guess = 'T';
				}
			}
		}

		if (guess != '!') txtinput[txtidx++] = guess;
		update_text();
	}
	if (event.type == ui::TouchEvent::Type::Move) {
		if (tracing) {
			current_pos = event.point;
		}
	}
	return true;
}

void HandWriteView::sample_pen() {
	int16_t diff_x, diff_y;
	uint8_t dir;
	
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
				
				display.fill_rectangle(
					{{current_pos.x, current_pos.y}, {4, 4}},
					Color::grey()
				);
				
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
							if ((dir & 0xF0) == 0x20) {
								if ((move_list[move_index - 1] & 0x0F) != (dir & 0x0F)) {
									move_list[move_index] = dir;
									move_index++;
								}
							} else if ((dir & 0x0F) == 0x02) {
								if ((move_list[move_index - 1] & 0xF0) != (dir & 0xF0)) {
									move_list[move_index] = dir;
									move_index++;
								}
							} else {
								// Replacement ?
								if (((move_list[move_index - 1] & 0xF0) == 0x20) && ((dir & 0xF0) != 0x20)) {
									if ((move_list[move_index - 1] & 0x0F) == (dir & 0x0F)) {
										move_list[move_index - 1] = dir;
									} else if ((dir & 0x0F) == 0x02) {
										// Merge
										move_list[move_index - 1] = (dir & 0xF0) | (move_list[move_index - 1] & 0x0F);
									} else {
										move_list[move_index] = dir;
										move_index++;
									}
								} else if (((move_list[move_index - 1] & 0x0F) == 0x02) && ((dir & 0x0F) != 0x02)) {
									if ((move_list[move_index - 1] & 0xF0) == (dir & 0xF0)) {
										move_list[move_index - 1] = dir;
									} else if ((dir & 0xF0) == 0x20) {
										// Merge
										move_list[move_index - 1] = (dir & 0x0F) | (move_list[move_index - 1] & 0xF0);
									} else {
										move_list[move_index] = dir;
										move_index++;
									}
								} else {
									move_list[move_index] = dir;
									move_index++;
								}
							}
						}
					} else {
						if (dir != 0x22) {
							move_list[move_index] = dir;
							move_index++;
						}
					}

					// DEBUG
					/*if (move_index) {
						memset(txtinput, 0, 20);
						txtidx = 0;
						for (i = 0; i < move_index; i++) {
							if ((move_list[i] & 0x03) == 0) char_add('L');
							if ((move_list[i] & 0x03) == 1) char_add('R');
							if ((move_list[i] & 0x03) == 2) char_add('?');
							if ((move_list[i] >> 4) == 0) char_add('U');
							if ((move_list[i] >> 4) == 1) char_add('D');
							if ((move_list[i] >> 4) == 2) char_add('?');
							char_add(' ');
						}
						update_text();
					}*/
					
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

void HandWriteView::char_delete() {
	if (txtidx) {
		txtidx--;
		txtinput[txtidx] = ' ';
	}
}

void HandWriteView::update_text() {
	text_input.set(txtinput);
}

}
