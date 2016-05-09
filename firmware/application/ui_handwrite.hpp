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

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "ui_menu.hpp"
#include "ui_navigation.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "clock_manager.hpp"
#include "message.hpp"
#include "signal.hpp"

namespace ui {

class HandWriteView : public View {
public:
	std::function<void(char *)> on_changed;

	HandWriteView(NavigationView& nav, char txt[], uint8_t max_len);
	~HandWriteView();

	void on_show() override;
	bool on_touch(const TouchEvent event) override;
	
	char * value();
	
	uint8_t txtidx;
	
	void char_add(const char c);
	void char_delete();

private:
	SignalToken signal_token_tick_second;
	
	uint8_t _max_len, txt_idx = 0;
	uint8_t dir_cnt = 0;
	uint8_t dir_prev;
	bool tracing = false;
	uint8_t move_index;
	uint8_t sample_skip, move_wait;
	uint8_t move_list[32];		// TODO: Cap !
	Point start_pos, current_pos, last_pos;
	bool _lowercase = false;
	static constexpr size_t button_w = 240 / 5;
	static constexpr size_t button_h = 28;
	char txtinput[21] = {0};	// DEBUG
	
	bool MM(uint8_t idx, char cmp);
	bool MM(uint8_t idx, char cmpud, char cmplr);
	bool MI(uint8_t idx);
	bool MLAST(char cmp);

	void sample_pen();
	
	Text text_input {
		{ 0, 0, 240, 16 }
	};
	
	Text text_debug_x {
		{ 0, 16, 32, 16 }
	};
	Text text_debug_y {
		{ 0, 32, 32, 16 }
	};
	Text text_debug_write {
		{ 80, 24, 150, 16 }
	};

	std::array<Button, 10> num_buttons;

	Button button_lowercase {
		{ 88+64+16, 270, 32, 24 },
		"UC"
	};

	Button button_done {
		{ 88, 270, 64, 24 },
		"Done"
	};

	void on_button(Button& button);

	void update_text();
};

} /* namespace ui */
