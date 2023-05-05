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

#ifndef __UI_TEXTENTRY_H__
#define __UI_TEXTENTRY_H__

#include "ui.hpp"
#include "ui_navigation.hpp"

namespace ui {

// A TextField is bound to a string reference and allows the string
// to be manipulated. The field itself does not provide the UI for
// setting the value. It provides the UI of rendering the text,
// a cursor, and an API to edit the string content.
class TextField : public Widget {
public:
	TextField(std::string& str, Point position, uint32_t length = 30)
		: TextField{str, 64, position, length} { }

	// Str: the string containing the content to edit.
	// Max_length: max length the string is allowed to use.
	// Position: the top-left corner of the control.
	// Length: the number of characters to display.
	//   - Characters are 8 pixels wide.
	//   - The screen can show 30 characters max.
	//   - The control is 16 pixels tall.
	TextField(std::string& str, size_t max_length, Point position, uint32_t length = 30);

	TextField(const TextField&) = delete;
	TextField(TextField&&) = delete;
	TextField& operator=(const TextField&) = delete;
	TextField& operator=(TextField&&) = delete;

	const std::string& value() const;

	void set_cursor(uint32_t pos);
	void set_insert_mode();
	void set_overwrite_mode();

	void char_add(char c);
	void char_delete();

	void paint(Painter& painter) override;

	bool on_key(const KeyEvent key) override;
	bool on_encoder(const EncoderEvent delta) override;
	bool on_touch(const TouchEvent event) override;

protected:
	std::string&   text_;
	size_t         max_length_;
	uint32_t       char_count_;
	uint32_t       cursor_pos_;
	bool           insert_mode_;
};

class TextEntryView : public View {
public:
	std::function<void(std::string&)> on_changed { };
	
	void focus() override;
	std::string title() const override { return "Text entry"; };

	void set_cursor(uint32_t pos);
	
protected:
	TextEntryView(NavigationView& nav, std::string& str, size_t max_length);
	
	TextEntryView(const TextEntryView&) = delete;
	TextEntryView(TextEntryView&&) = delete;
	TextEntryView& operator=(const TextEntryView&) = delete;
	TextEntryView& operator=(TextEntryView&&) = delete;

	void char_add(const char c);
	void char_delete();
	
	TextField text_input;
	Button button_ok {
		{ 10 * 8, 33 * 8, 9 * 8, 32 },
		"OK"
	};
};

// Show the TextEntry view to receive keyboard input.
// NB: This function returns immediately. 'str' is taken
// by reference and its lifetime must be ensured by the
// caller until the TextEntry view is dismissed.
void text_prompt(
	NavigationView& nav,
	std::string& str,
	size_t max_length,
	std::function<void(std::string&)> on_done = nullptr);

void text_prompt(
	NavigationView& nav,
	std::string& str,
	uint32_t cursor_pos,
	size_t max_length,
	std::function<void(std::string&)> on_done = nullptr);

} /* namespace ui */

#endif/*__UI_TEXTENTRY_H__*/
