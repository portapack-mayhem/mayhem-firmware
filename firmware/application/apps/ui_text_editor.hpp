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
//#include "ui_menu.hpp"
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

struct FileInfo {
    /* Offsets of newlines. */
    std::vector<uint32_t> newlines;
};

class TextEditorView : public View {
public:

	TextEditorView(NavigationView& nav);
	//TextEditorView(NavigationView& nav, const std::filesystem::path& path);

    std::string title() const override {
        return "Text Editor";
    };

private:
    NavigationView& nav_;
    File file_{ };

    LogFile log_{ };

    /* 8px grid is 30 wide, 36 tall */
    /* 240 x 288px */

    Text text_position {
        { 0 * 8, 36 * 8, 30 * 8, 2 * 8 },
        ""
    };
};

} // namespace ui

#endif // __UI_TEXT_EDITOR_H__