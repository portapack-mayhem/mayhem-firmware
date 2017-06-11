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
#include <algorithm>

using namespace hackrf::one;

namespace ui {

void AlphanumView::paint(Painter&) {
	draw_cursor();
}

AlphanumView::AlphanumView(
	NavigationView& nav,
	std::string * str,
	size_t max_length
) : _max_length(max_length),
	_str(str)
{
	size_t n;
	
	// Trim from right
	_str->erase(std::find_if(_str->rbegin(), _str->rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), _str->end());
	cursor_pos = _str->length();
	
	_str->resize(_max_length, 0);
	
	add_children({
		&text_input,
		&button_mode,
		&text_raw,
		&field_raw,
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
			static_cast<Coord>((n / 5) * 38 + 24),
			240 / 5, 38
		});
		add_child(&button);
		n++;
	}
	
	set_mode(mode);
	
	button_mode.on_select = [this](Button&) {
		set_mode(mode + 1);
	};
	
	field_raw.set_value('0');
	field_raw.on_select = [this](NumberField&) {
		char_add(field_raw.value());
		update_text();
	};

	button_ok.on_select = [this, &nav](Button&) {
		if (on_changed)
			on_changed(_str);
		nav.pop();
	};

	update_text();
}

void AlphanumView::draw_cursor() {
	Point draw_pos;
	
	draw_pos = {text_input.screen_rect().location().x() + 8 + std::min((Coord)cursor_pos, (Coord)28) * 8,
					text_input.screen_rect().location().y() + 16};
	
	portapack::display.fill_rectangle(
		{{text_input.screen_rect().location().x(), draw_pos.y()}, {text_input.screen_rect().size().width(), 4}},
		Color::black()
	);
	portapack::display.fill_rectangle(
		{draw_pos, {8, 4}},
		Color::white()
	);
}

void AlphanumView::set_mode(const uint32_t new_mode) {
	size_t n = 0;
	
	if (new_mode < 3)
		mode = new_mode;
	else
		mode = 0;
	
	const char * key_list = key_sets[mode].second;
	
	for (auto& button : buttons) {
		const std::string label {
			key_list[n]
		};
		button.set_text(label);
		n++;
	}
	
	if (mode < 2)
		button_mode.set_text(key_sets[mode + 1].first);
	else
		button_mode.set_text(key_sets[0].first);
}

void AlphanumView::focus() {
	button_ok.focus();
}

void AlphanumView::on_button(Button& button) {
	const auto c = button.text()[0];
	
	if (c == '<')
		char_delete();
	else
		char_add(c);
	
	update_text();
}

void AlphanumView::char_add(const char c) {
	if (cursor_pos >= _max_length) return;
	
	_str->replace(cursor_pos, 1, 1, c);
	cursor_pos++;
}

void AlphanumView::char_delete() {
	if (!cursor_pos) return;
	
	cursor_pos--;
	_str->replace(cursor_pos, 1, 1, 0);
}

void AlphanumView::update_text() {
	if (cursor_pos <= 28)
		text_input.set(' ' + *_str + std::string(_max_length - _str->length(), ' '));
	else
		text_input.set('<' + _str->substr(cursor_pos - 28, 28));
		
	draw_cursor();
}

}
