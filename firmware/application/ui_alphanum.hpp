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
#include "rf_path.hpp"
#include "max2837.hpp"
#include "volume.hpp"
#include "transmitter_model.hpp"

namespace ui {

class AlphanumView : public View {
public:
	std::function<void(char *)> on_changed;

	AlphanumView(NavigationView& nav, char txt[], uint8_t max_len);

	void focus() override;
	
	char * value();
	
	uint8_t txtidx;
	
	void char_add(const char c);
	void char_delete();

private:
	uint8_t _max_len;
	bool _lowercase = false;
	static constexpr size_t button_w = 240 / 5;
	static constexpr size_t button_h = 28;
	char txtinput[9];
	
	void set_lowercase();
	void set_uppercase();
	
	Text text_input {
		{ 88, 0, 240, 16 }
	};

	std::array<Button, 40> buttons;

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
