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

struct FileInfo {
    /* Offsets of newlines. */
    std::vector<uint32_t> newlines;
    LineEnding line_ending;
    File::Size size;

    uint32_t line_count() const {
        return newlines.size();
    }

    uint16_t line_length(uint32_t line) const {
        if (line >= line_count())
            return 0;

        auto start = line == 0 ? 0 : (newlines[line - 1] + 1);
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
    
    void paint(Painter& painter) override;
    bool on_key(KeyEvent delta) override;
    bool on_encoder(EncoderEvent delta) override;

private:
    static constexpr uint8_t max_line = 15;
    static constexpr uint8_t max_col = 29;

    // TODO: should these be common somewhere?
    static constexpr Style style_default {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::white(),
	};

    void refresh_ui();
    void refresh_file_info();
    std::string read(uint32_t offset, uint32_t length = 30);

    void paint_text(Painter& painter, uint32_t line, uint16_t col);
    void paint_cursor(Painter& painter);

    // Gets the length of the current line;
    uint16_t line_length() const;

    NavigationView& nav_;

    File     file_{ };
    FileInfo info_{ };
    LogFile  log_{ };

    struct {
        uint32_t line{ };
        uint16_t col{ };
        uint32_t first_line{ };
        uint32_t first_col{ };
        bool redraw_text{ true };
    } paint_state_{ };

    struct {
        uint32_t line{ };
        uint16_t col{ };
        ScrollDirection dir{ ScrollDirection::Vertical };
    } cursor_{ };

    /* 8px grid is 30 wide, 36 tall. */
    /* 16px font height or 18 total rows. */
    /* 240 x 288px */

    Text text_position {
        { 0 * 8, 36 * 8, 30 * 8, 2 * 8 },
        ""
    };
};

} // namespace ui

#endif // __UI_TEXT_EDITOR_H__