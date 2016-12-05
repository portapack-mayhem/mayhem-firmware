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

#include "ui_alphanum.hpp"

#include "ch.h"

#include "ff.h"
#include "portapack.hpp"
#include "radio.hpp"

#include "hackrf_hal.hpp"
#include "portapack_shared_memory.hpp"

#include <cstring>

using namespace hackrf::one;

namespace ui {
	
void AlphanumView::paint(Painter& painter) {
	(void)painter;
	move_cursor();
}
	
AlphanumView::AlphanumView(
	NavigationView& nav,
	char txt[],
	uint8_t max_len
) {
	_max_len = max_len;
	_lowercase = false;
	size_t n;
	
	static constexpr Style style_alpha {
		.font = font::fixed_8x16,
		.background = Color(191,31,31),
		.foreground = Color::black()
	};
	
	static constexpr Style style_num {
		.font = font::fixed_8x16,
		.background = Color(191,191,31),
		.foreground = Color::black()
	};
	
	txtidx = strlen(txt);
	memcpy(txtinput, txt, max_len + 1);
	n = txtidx;
	while (n && (txtinput[n - 1] == ' ')) {
		txtinput[--n] = 0;
		txtidx--;
	}
	
	add_child(&text_input);

	const auto button_fn = [this](Button& button) {
		this->on_button(button);
	};

	n = 0;
	for (auto& button : buttons) {
		button.on_select = button_fn;
		button.set_parent_rect({
			static_cast<Coord>((n % 5) * button_w),
			static_cast<Coord>((n / 5) * button_h + 24),
			button_w, button_h
		});
		if ((n < 10) || (n == 39))
			button.set_style(&style_num);
		else
			button.set_style(&style_alpha);
		add_child(&button);
		n++;
	}
	set_uppercase();
	
	add_child(&button_lowercase);
	button_lowercase.on_select = [this, &nav, max_len](Button&) {
		if (_lowercase == true) {
			_lowercase = false;
			button_lowercase.set_text("UC");
			set_uppercase();
		} else {
			_lowercase = true;
			button_lowercase.set_text("LC");
			set_lowercase();
		}
	};
	
	add_child(&raw_char);
	raw_char.set_value(0x30);
	raw_char.on_select = [this, &nav, txt, max_len](NumberField&) {
		char_add(raw_char.value());
		update_text();
	};

	add_child(&button_done);
	button_done.on_select = [this, &nav, txt, max_len](Button&) {
		memcpy(txt, txtinput, max_len + 1);
		if (on_changed) on_changed(this->value());
		nav.pop();
	};

	update_text();
}

void AlphanumView::move_cursor() {
	Point cursor_pos;
	
	cursor_pos.x = text_input.screen_rect().pos.x + (txtidx * 8);
	cursor_pos.y = text_input.screen_rect().pos.y + 16;
	
	portapack::display.fill_rectangle(
		{{text_input.screen_rect().pos.x, cursor_pos.y}, {text_input.screen_rect().size.w, 4}},
		Color::black()
	);
	portapack::display.fill_rectangle(
		{cursor_pos, {8, 4}},
		Color::white()
	);
}

void AlphanumView::set_uppercase() {
	const char* const key_caps = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ. !<";

	size_t n = 0;
	for(auto& button : buttons) {
		//add_child(&button);
		const std::string label {
			key_caps[n]
		};
		button.set_text(label);
		n++;
	}
}


void AlphanumView::set_lowercase() {
	const char* const key_caps = "0123456789abcdefghijklmnopqrstuvwxyz:=?<";

	size_t n = 0;
	for(auto& button : buttons) {
		//add_child(&button);
		const std::string label {
			key_caps[n]
		};
		button.set_text(label);
		n++;
	}
}

void AlphanumView::focus() {
	button_done.focus();
}

char * AlphanumView::value() {
	txtinput[txtidx] = 0;
	return txtinput;
}

void AlphanumView::on_button(Button& button) {
	const auto s = button.text();
	if( s == "<" ) {
		char_delete();
	} else {
		char_add(s[0]);
	}
	update_text();
}

void AlphanumView::char_add(const char c) {
	if (txtidx < _max_len) {
		txtinput[txtidx] = c;
		txtidx++;
	}
}

void AlphanumView::char_delete() {
	if (txtidx) {
		txtidx--;
		txtinput[txtidx] = 0;
	}
}

void AlphanumView::update_text() {
	text_input.set(std::string(txtinput) + std::string(_max_len - strlen(txtinput), ' '));
	move_cursor();
}

}
