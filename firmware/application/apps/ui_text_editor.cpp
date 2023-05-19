/*
 * Copyright (C) 2023 Kyle Reed
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

 #include "ui_fileman.hpp"
 #include "ui_text_editor.hpp"
 #include "string_format.hpp"

 using namespace portapack;
namespace fs = std::filesystem;

 namespace {
	template <typename T>
	T mid(const T& val1, const T& val2, const T& val3) {
		return std::max(val1, std::min(val2, val3));
	} 
 } // namespace

 namespace ui {

TextEditorView::TextEditorView(NavigationView& nav)
	: nav_{ nav }
{
	add_children({
		&button_open,
		&text_position
});
	set_focusable(true);

	//log_.append("LOGS/NOTEPAD.TXT");

	button_open.on_select = [this](Button&) {
		auto open_view = nav_.push<FileLoadView>(".TXT");
		open_view->on_changed = [this](std::filesystem::path path) {
			open_file(path);
		};
	};
}

void TextEditorView::on_focus() {
	refresh_ui();
}

void TextEditorView::paint(Painter& painter) {
	auto first_line = paint_state_.first_line;
	auto first_col = paint_state_.first_col;

	if (!paint_state_.has_file)
		return;

	if (cursor_.line < first_line)
		first_line = cursor_.line;
	else if (cursor_.line >= first_line + max_line)
		first_line = cursor_.line - max_line + 1;
		
	if (cursor_.col < first_col)
		first_col = cursor_.col;
	if (cursor_.col >= first_col + max_col)
		first_col = cursor_.col - max_col + 1;

	if (first_line != paint_state_.first_line ||
		first_col != paint_state_.first_col) {
		paint_state_.first_line = first_line;
		paint_state_.first_col = first_col;
		paint_state_.redraw_text = true;
	}

	if (paint_state_.redraw_text) {
		paint_text(painter, first_line, first_col);
		paint_state_.redraw_text = false;
	}

	paint_cursor(painter);
}

bool TextEditorView::on_key(const KeyEvent key) {
	int16_t delta_col = 0;
	int16_t delta_line = 0;

	if (key == KeyEvent::Left)
		delta_col = -1;
	else if (key == KeyEvent::Right)
		delta_col = 1;
	else if (key == KeyEvent::Up)
		delta_line = -1;
	else if (key == KeyEvent::Down)
		delta_line = 1;
	/*	else if (key == KeyEvent::Select)
		; // TODO: Edit/Menu */

	// Always allow cursor direction to be updated.
	cursor_.dir = delta_col != 0 ? ScrollDirection::Horizontal : ScrollDirection::Vertical;
	auto updated = apply_scrolling_constraints(delta_line, delta_col);
	
	if (updated)
		refresh_ui();
	return updated;
}

bool TextEditorView::on_encoder(EncoderEvent delta) {
	bool updated = false;

	if (cursor_.dir == ScrollDirection::Horizontal)
		updated = apply_scrolling_constraints(0, delta);
	else
		updated = apply_scrolling_constraints(delta, 0);

	if (updated)
		refresh_ui();

	return updated;
}

bool TextEditorView::apply_scrolling_constraints(int16_t delta_line, int16_t delta_col) {
	int32_t new_line = cursor_.line + delta_line;
	int32_t new_col = cursor_.col + delta_col;

	auto new_line_length = info_.line_length(new_line);

	if (new_col < 0)
		--new_line;
	else if (new_col > new_line_length && delta_line == 0) {
		// Only want to wrap if moving horizontally.
		new_col = 0;
		++new_line;
	}

	if (new_line < 0 || (uint32_t)new_line >= info_.line_count())
		return false;

	new_line_length = info_.line_length(new_line);

	// TODO: don't wrap with encoder?
	// Wrap or clamp column.
	if (new_line_length == 0)
		new_col = 0;
	else if (new_col > new_line_length || new_col < 0)
		new_col = new_line_length;

	cursor_.line = new_line;
	cursor_.col = new_col;

	return true;
}

void TextEditorView::refresh_ui() {
	if (paint_state_.has_file) {
		text_position.set(
			to_string_dec_uint(cursor_.col + 1) + ":" +
			to_string_dec_uint(cursor_.line + 1) + "/" +
			to_string_dec_uint(info_.line_count()) +
			(info_.truncated ? "*" : "") +
			" Size: " +
			to_string_file_size(info_.size));
		focus();
	} else {
		button_open.focus();
	}

	set_dirty();
}

void TextEditorView::refresh_file_info() {
	constexpr size_t buffer_size = 128;
	char buffer[buffer_size];
	uint32_t base_offset = 0;

	file_.seek(0);
	info_.newlines.clear();
	info_.line_ending = LineEnding::LF;
	info_.size = file_.size();
	info_.truncated = false;

	while (true) {
		auto result = file_.read(buffer, buffer_size);
		if (result.is_error())
			break; // TODO: report error?

		// TODO: CRLF state machine for cross block.
		for (uint32_t i = 0; i < result.value(); ++i) {
			switch (buffer[i]) { 
				case '\n':
					info_.newlines.push_back(base_offset + i);
			}
		}

		base_offset += result.value();

		// Fake a newline at the end for consistency.
		// Could check if there already is a trailing newline, but it doesn't hurt.
		if (result.value() < buffer_size) {
			info_.newlines.push_back(base_offset);
			break;
		}

		// HACK HACK: only show first 1000 lines for now.
		if (info_.newlines.size() >= 1000) {
			info_.truncated = true;
			break;
		}
	}

	refresh_ui();
}

void TextEditorView::open_file(const fs::path& path) {
	// TODO: need a temp backing file for edits.

	auto result = file_.open(path);
	paint_state_.has_file = !result.is_valid(); /* Has an error. */

	if (paint_state_.has_file) {
		refresh_file_info();
		paint_state_.first_line = 0;
		paint_state_.first_col = 0;
		cursor_.line = 0;
		cursor_.col = 0;
	} else {
		nav_.display_modal("Read Error", "Cannot open file:\n" + result.value().what());
		paint_state_.has_file = false;
	}
	
	paint_state_.redraw_text = true;
	refresh_ui();
}

std::string TextEditorView::read(uint32_t offset, uint32_t length) {
	if (offset >= info_.size)
		return { "[BAD OFFSET]" };

	std::string buffer(length + 1, '\0');
	file_.seek(offset);

	auto result = file_.read(&buffer[0], length);
	if (result.is_ok())
		/* resize? */;
	else
		return result.error().what();
	
	return buffer;
}

void TextEditorView::paint_text(Painter& painter, uint32_t line, uint16_t col) {
	// TODO: A line cache would use more memory but save a lot of IO.
	// Only the new lines/characters would need to be refetched.

	auto r = screen_rect();
	auto& lines = info_.newlines;
	auto line_start = info_.line_start(line);

	// Draw the lines from the file
	for (uint32_t i = 0; i < max_line && i < lines.size(); ++i) {
		auto line_end = lines[line + i];
		int32_t read_length = max_col;

		// Don't read past end of the line.
		if (line_start + col + (uint32_t)read_length > line_end)
			read_length = line_end - col - line_start;

		if (read_length > 0)
			painter.draw_string(
				{ 0, r.location().y() + (int)i * char_height },
				style_default, read(line_start + col, read_length));

		// Erase empty line sectons.
		if (read_length >= 0) {
			int32_t clear_width = max_col - read_length;
			if (clear_width > 0)
				painter.fill_rectangle({
					(max_col - clear_width) * char_width,
					r.location().y() + (int)i * char_height,
					clear_width * char_width, char_height },
					style_default.background);
		}

		line_start = lines[line + i] + 1 /* newline */;
	}
}

void TextEditorView::paint_cursor(Painter& painter) {
	auto draw_cursor = [this, &painter](uint32_t line, uint16_t col, Color c) {
		auto r = screen_rect();
		line = line - paint_state_.first_line;
		col = col - paint_state_.first_col;

		painter.draw_rectangle({
			(int)col * char_width - 1,
			r.location().y() + (int)line * char_height,
			char_width + 1, char_height}, c);
	};

	// TOOD: bug where cursor doesn't clear at EOF.
	// TODO: XOR cursor?

	// Clear old cursor.
	draw_cursor(paint_state_.line, paint_state_.col, style_default.background);
	draw_cursor(cursor_.line, cursor_.col, style_default.foreground);
	paint_state_.line = cursor_.line;
	paint_state_.col = cursor_.col;
}

uint16_t TextEditorView::line_length() const {
	return info_.line_length(cursor_.line);
}

} // namespace ui