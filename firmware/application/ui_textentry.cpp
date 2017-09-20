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

#include "ui_textentry.hpp"
//#include "portapack_persistent_memory.hpp"
#include "ui_alphanum.hpp"
//#include "ui_handwrite.hpp"

using namespace portapack;

namespace ui {

void text_prompt(NavigationView& nav, std::string * str, const size_t max_length, const std::function<void(std::string *)> on_done) {
	//if (persistent_memory::ui_config_textentry() == 0) {
		auto te_view = nav.push<AlphanumView>(str, max_length);
		te_view->on_changed = [on_done](std::string * value) {
			if (on_done)
				on_done(value);
		};
	/*} else {
		auto te_view = nav.push<HandWriteView>(str, max_length);
		te_view->on_changed = [on_done](std::string * value) {
			if (on_done)
				on_done(value);
		};
	}*/
}

void TextEntryView::update_text() {
	if (_cursor_pos <= 28)
		text_input.set(' ' + *_str + std::string(_max_length - _str->length(), ' '));
	else
		text_input.set('<' + _str->substr(_cursor_pos - 28, 28));
		
	draw_cursor();
}

void TextEntryView::char_delete() {
	if (!_cursor_pos) return;
	
	_cursor_pos--;
	_str->replace(_cursor_pos, 1, 1, 0);
}

void TextEntryView::char_add(const char c) {
	if (_cursor_pos >= _max_length) return;
	
	_str->replace(_cursor_pos, 1, 1, c);
	_cursor_pos++;
}

void TextEntryView::draw_cursor() {
	Point draw_pos;
	
	draw_pos = { text_input.screen_rect().location().x() + 8 + std::min((Coord)_cursor_pos, (Coord)28) * 8,
					text_input.screen_rect().location().y() + 16 };
	
	display.fill_rectangle(
		{ { text_input.screen_rect().location().x(), draw_pos.y() }, { text_input.screen_rect().size().width(), 4 } },
		Color::black()
	);
	display.fill_rectangle(
		{ draw_pos, { 8, 4 } },
		Color::white()
	);
}

void TextEntryView::focus() {
	button_ok.focus();
}

TextEntryView::TextEntryView(
	NavigationView& nav,
	std::string * str,
	size_t max_length
) : _str(str),
	_max_length(max_length)
{
	
	// Trim from right
	_str->erase(std::find_if(_str->rbegin(), _str->rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), _str->end());
	_cursor_pos = _str->length();
	
	_str->reserve(_max_length);
	
	add_children({
		&text_input,
		&button_ok
	});
	
	button_ok.on_select = [this, &nav](Button&) {
		_str->substr(0, _cursor_pos);
		if (on_changed)
			on_changed(_str);
		nav.pop();
	};
}

} /* namespace ui */
