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

#ifndef __UI_TEXT_EDITOR_H__
#define __UI_TEXT_EDITOR_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "ui_navigation.hpp"
//#include "ui_textentry.hpp"

#include "file.hpp"
#include "log_file.hpp"

#include <string>
#include <vector>

namespace ui {

enum class LineEnding : uint8_t {
	LF,
	CRLF
};

enum class ScrollDirection : uint8_t {
	Vertical,
	Horizontal
};

// TODO: RAM is _very_ limited. Need to
// rework this to not store every line.
// Should be able to get away with only
// abount one screen of lines so long as
// you can't scroll more than one screen
// at a time.
struct FileInfo {
	/* Offsets of newlines. */
	std::vector<uint32_t> newlines;
	LineEnding line_ending;
	File::Size size;
	bool truncated;

	uint32_t line_count() const {
		return newlines.size();
	}

	uint32_t line_start(uint32_t line) const {
		return line == 0 ? 0 : (newlines[line - 1] + 1);
	}

	uint16_t line_length(uint32_t line) const {
		if (line >= line_count())
			return 0;

		auto start = line_start(line);
		auto end = newlines[line];
		return end - start;
	}
};

/*class TextViewer : public Widget {
};*/

class TextEditorView : public View {
public:
	TextEditorView(NavigationView& nav);
	//TextEditorView(NavigationView& nav, const std::filesystem::path& path);

	std::string title() const override {
		return "Notepad";
	};
	
	void on_focus() override;
	void paint(Painter& painter) override;
	bool on_key(KeyEvent delta) override;
	bool on_encoder(EncoderEvent delta) override;

private:
	static constexpr uint8_t max_line = 16;
	static constexpr uint8_t max_col = 30;

	// TODO: should these be common somewhere?
	static constexpr Style style_default {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::white(),
	};

	/* Returns true if the cursor was updated. */
	bool apply_scrolling_constraints(
		int16_t delta_line, int16_t delta_col);

	void refresh_ui();
	void refresh_file_info();
	void open_file(const std::filesystem::path& path);
	std::string read(uint32_t offset, uint32_t length = 30);

	void paint_text(Painter& painter, uint32_t line, uint16_t col);
	void paint_cursor(Painter& painter);

	// Gets the length of the current line.
	uint16_t line_length() const;

	NavigationView& nav_;

	File     file_{ };
	FileInfo info_{ };
	//LogFile  log_{ };

	struct {
		// Previous cursor state.
		uint32_t line{ };
		uint16_t col{ };

		// Previous draw state.
		uint32_t first_line{ };
		uint16_t first_col{ };
		bool redraw_text{ true };
		bool has_file{ false };
	} paint_state_{ };

	struct {
		uint32_t line{ };
		uint16_t col{ };
		ScrollDirection dir{ ScrollDirection::Vertical };
	} cursor_{ };

	/* 8px grid is 30 wide, 38 tall. */
	/* 16px font height or 19 rows. */
	/* Titlebar is 16px tall, so 18 rows left. */
	/* 240 x 320, (304 with titlebar) */

	// TODO: The scrollable view should be its own widget
	// otherwise control navigation doesn't work.

	Button button_open {
		{ 19 * 8, 34 * 8, 11 * 8, 4 * 8 },
		"Open File"
	};

	Text text_position {
		{ 0 * 8, 36 * 8, 19 * 8, 2 * 8 },
		""
	};
};

} // namespace ui

#endif // __UI_TEXT_EDITOR_H__