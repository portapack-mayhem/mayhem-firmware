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

#include "portapack.hpp"
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
	size_t max_length
) : _max_length(max_length)
{
	_lowercase = false;
	size_t n;
	
	static constexpr Style style_alpha {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color(191,31,31)
	};
	
	static constexpr Style style_num {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color(191,191,31)
	};
	
	txtidx = strlen(txt);
	memcpy(txtinput, txt, _max_length + 1);
	n = txtidx;
	while (n && (txtinput[n - 1] == ' ')) {
		txtinput[--n] = 0;
		txtidx--;
	}
	
	add_children({
		&text_input,
		&button_lowercase,
		&raw_char,
		&button_ok
	});

	const auto button_fn = [this](Button& button) {
		this->on_button(button);
	};

	n = 0;
	for (auto& button : buttons) {
		button.on_select = button_fn;
		button.set_parent_rect({
			static_cast<Coord>((n % 5) * (240 / 5)),
			static_cast<Coord>((n / 5) * 28 + 24),
			240 / 5, 28
		});
		if ((n < 10) || (n == 39))
			button.set_style(&style_num);
		else
			button.set_style(&style_alpha);
		add_child(&button);
		n++;
	}
	set_keys(keys_upper);
	
	button_lowercase.on_select = [this, &nav](Button&) {
		if (_lowercase == true) {
			_lowercase = false;
			button_lowercase.set_text("UC");
			set_keys(keys_upper);
		} else {
			_lowercase = true;
			button_lowercase.set_text("LC");
			set_keys(keys_lower);
		}
	};
	
	raw_char.set_value('0');
	raw_char.on_select = [this, &nav](NumberField&) {
		char_add(raw_char.value());
		update_text();
	};

	button_ok.on_select = [this, &nav, txt, max_length](Button&) {
		memcpy(txt, txtinput, max_length + 1);
		if (on_changed) on_changed(this->value());
		nav.pop();
	};

	update_text();
}

void AlphanumView::move_cursor() {
	Point cursor_pos;
	
	cursor_pos = {text_input.screen_rect().location().x() + (txtidx * 8),
					text_input.screen_rect().location().y() + 16};
	
	portapack::display.fill_rectangle(
		{{text_input.screen_rect().location().x(), cursor_pos.y()}, {text_input.screen_rect().size().width(), 4}},
		Color::black()
	);
	portapack::display.fill_rectangle(
		{cursor_pos, {8, 4}},
		Color::white()
	);
}

void AlphanumView::set_keys(const char * const key_list) {
	size_t n = 0;
	
	for (auto& button : buttons) {
		const std::string label {
			key_list[n]
		};
		button.set_text(label);
		n++;
	}
}

void AlphanumView::focus() {
	button_ok.focus();
}

char * AlphanumView::value() {
	txtinput[txtidx] = 0;
	return txtinput;
}

void AlphanumView::on_button(Button& button) {
	const auto s = button.text();
	
	if (s == "<")
		char_delete();
	else
		char_add(s[0]);
	
	update_text();
}

void AlphanumView::char_add(const char c) {
	if (txtidx >= _max_length) return;
	
	txtinput[txtidx] = c;
	txtidx++;
}

void AlphanumView::char_delete() {
	if (!txtidx) return;
	
	txtidx--;
	txtinput[txtidx] = 0;
}

void AlphanumView::update_text() {
	text_input.set(std::string(txtinput) + std::string(_max_length - strlen(txtinput), ' '));
	move_cursor();
}

}
