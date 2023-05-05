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

void text_prompt(
	NavigationView& nav,
	std::string& str,
	size_t max_length,
	std::function<void(std::string&)> on_done
) {
	text_prompt(nav, str, str.length(), max_length, on_done);
}

void text_prompt(
	NavigationView& nav,
	std::string& str,
	uint32_t cursor_pos,
	size_t max_length,
	std::function<void(std::string&)> on_done
) {
	//if (persistent_memory::ui_config_textentry() == 0) {
		auto te_view = nav.push<AlphanumView>(str, max_length);
		te_view->set_cursor(cursor_pos);
		te_view->on_changed = [on_done](std::string& value) {
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

/* TextField ***********************************************************/

TextField::TextField(
	std::string& str,
	size_t max_length,
	Point position,
	uint32_t length
) : Widget{ { position, { 8 * static_cast<int>(length), 16 } } },
	text_{ str },
	max_length_{ std::max<size_t>(max_length, str.length()) },
	char_count_{ std::max<uint32_t>(length, 1) },
	cursor_pos_{ text_.length() },
	insert_mode_{ true }
{
	set_focusable(true);
}

const std::string& TextField::value() const {
	return text_;
}

void TextField::set_cursor(uint32_t pos) {
	cursor_pos_ = std::min<size_t>(pos, text_.length());
	set_dirty();
}

void TextField::set_insert_mode() {
	insert_mode_ = true;
}

void TextField::set_overwrite_mode() {
	insert_mode_ = false;
}

void TextField::char_add(char c) {
	// Don't add if inserting and at max_length and
	// don't overwrite if past the end of the text.
	if ((text_.length() >= max_length_ && insert_mode_) ||
		(cursor_pos_ >= text_.length() && !insert_mode_))
		return;

	if (insert_mode_)
		text_.insert(cursor_pos_, 1, c);
	else
		text_[cursor_pos_] = c;

	cursor_pos_++;
	set_dirty();
}

void TextField::char_delete() {
	if (cursor_pos_ == 0)
		return;

	cursor_pos_--;
	text_.erase(cursor_pos_, 1);
	set_dirty();
}

void TextField::paint(Painter& painter) {
	constexpr int char_width = 8;

	auto rect = screen_rect();
	auto text_style = has_focus() ? style().invert() : style();
	auto offset = 0;

	// Does the string need to be shifted?
	if (cursor_pos_ >= char_count_)
		offset = cursor_pos_ - char_count_ + 1;

	// Clear the control.
	painter.fill_rectangle(rect, text_style.background);

	// Draw the text starting at the offset.
	for (uint32_t i = 0; i < char_count_ && i + offset < text_.length(); i++) {
		painter.draw_char(
			{ rect.location().x() + (static_cast<int>(i) * char_width), rect.location().y() },
			text_style,
			text_[i + offset]
		);
	}

	// Determine cursor position on screen (either the cursor position or the last char).
	int32_t cursor_x = char_width * (offset > 0 ? char_count_ - 1 : cursor_pos_);
	Point cursor_point{ screen_pos().x() + cursor_x, screen_pos().y() };
	auto cursor_style = text_style.invert();

	// Invert the cursor character when in overwrite mode.
	if (!insert_mode_ && (cursor_pos_) < text_.length())
		painter.draw_char(cursor_point, cursor_style, text_[cursor_pos_]);

	// Draw the cursor.
	Rect cursor_box{ cursor_point, { char_width, 16 } };
	painter.draw_rectangle(cursor_box, cursor_style.background);
}

bool TextField::on_key(const KeyEvent key) {
	if (key == KeyEvent::Left && cursor_pos_ > 0)
		cursor_pos_--;
	else if (key == KeyEvent::Right && cursor_pos_ < text_.length())
		cursor_pos_++;
	else if (key == KeyEvent::Select)
		insert_mode_ = !insert_mode_;
	else
		return false;

	set_dirty();
	return true;
}

bool TextField::on_encoder(const EncoderEvent delta) {
	int32_t new_pos = cursor_pos_ + delta;

	// Let the encoder wrap around the ends of the text.
	if (new_pos < 0)
		new_pos = text_.length();
	else if (static_cast<size_t>(new_pos) > text_.length())
		new_pos = 0;

	set_cursor(new_pos);
	return true;
}

bool TextField::on_touch(const TouchEvent event) {
	if (event.type == TouchEvent::Type::Start)
		focus();

	set_dirty();
	return true;
}

/* TextEntryView ***********************************************************/

void TextEntryView::char_delete() {
	text_input.char_delete();
}

void TextEntryView::char_add(const char c) {
	text_input.char_add(c);
}

void TextEntryView::set_cursor(uint32_t pos) {
	text_input.set_cursor(pos);
}

void TextEntryView::focus() {
	text_input.focus();
}

TextEntryView::TextEntryView(
	NavigationView& nav,
	std::string& str,
	size_t max_length
) : text_input{ str, max_length, { 0, 0 } }
{
	add_children({
		&text_input,
		&button_ok
	});

	button_ok.on_select = [this, &str, &nav](Button&) {
		if (on_changed)
			on_changed(str);
		nav.pop();
	};
}

} /* namespace ui */
