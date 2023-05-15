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

 #include "ui_text_editor.hpp"
 #include "string_format.hpp"

 using namespace portapack;
namespace fs = std::filesystem;

 namespace {
    template <typename T>
    T mid(const T& min_val, const T& max_val, const T& val) {
        return std::max(min_val, std::min(max_val, val));
    } 
 } // namespace

 namespace ui {

TextEditorView::TextEditorView(NavigationView& nav)
    : View{{ 0, 0, 8 * 30, 8 * 30 }}, nav_{ nav }
{
    add_children({
        &text_position
});
    set_focusable(true);

    // TODO: need a temp backing file.
    log_.append("TEXTEDITOR.TXT");

    log_.write_entry("Opening file");
    auto result = file_.open("POCSAG.TXT");
    if (result.is_valid()) {
        text_position.set(result.value().what());
        return;
    }

    refresh_file_info();
}

void TextEditorView::paint(Painter& painter) {
    auto first_line = 0u;
    auto first_col = 0u;

    if (cursor_.line >= max_line)
        first_line = cursor_.line - max_line;

    if (cursor_.col >= max_col)
        first_col = cursor_.col - max_col;

    if (first_line != paint_state_.line || first_col != paint_state_.col) {
        paint_state_.line = first_line;
        paint_state_.col = first_col;
        paint_state_.redraw_text = true;
    }

    if (paint_state_.redraw_text) {
        paint_text(painter, first_line, first_col);
        paint_state_.redraw_text = false;
    }

    paint_cursor(painter);
}

bool TextEditorView::on_key(const KeyEvent key) {
    int delta_col = 0;
    int delta_line = 0;

	if (key == KeyEvent::Left && cursor_.col > 0)
		delta_col = -1;
	else if (key == KeyEvent::Right && cursor_.col < line_length())
		delta_col = 1;
    else if (key == KeyEvent::Up && cursor_.line > 0)
        delta_line = -1;
    else if (key == KeyEvent::Down && cursor_.line < info_.line_count())
        delta_line = 1;
/*	else if (key == KeyEvent::Select)
		; // TODO: Edit/Menu */
	else
		return false;

    if (delta_col != 0) {
        cursor_.dir = ScrollDirection::Horizontal;
        cursor_.col += delta_col;
    }
    if (delta_line != 0) {
        cursor_.dir = ScrollDirection::Vertical;
        cursor_.line += delta_line;
        if (cursor_.col >= line_length())
            cursor_.col = line_length() - 1;
    }
    
    refresh_ui();
	return true;
}

bool TextEditorView::on_encoder(EncoderEvent delta) {
    if (cursor_.dir == ScrollDirection::Horizontal) {
        uint16_t new_col = cursor_.col + delta;
    
        if (new_col >= line_length()) // NB: Underflow wrap.
            return false;

        cursor_.col = new_col;
    } else {
        uint32_t new_line = cursor_.line + delta;
    
        if (new_line >= info_.line_count()) // NB: Underflow wrap.
            return false;

        cursor_.line = new_line;
    }

    refresh_ui();
    return true;
}

void TextEditorView::refresh_ui() {
        text_position.set(
        to_string_dec_uint(cursor_.col + 1) + ":" +
        to_string_dec_uint(cursor_.line + 1) + "/" +
        to_string_dec_uint(info_.line_count()) +
        " Size: " +
        to_string_file_size(info_.size));

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
            info_.newlines.push_back(base_offset );
            break;
        }
    }

    refresh_ui();
}

std::string TextEditorView::read(uint32_t offset, uint32_t length) {
    // Where should the EOL check live?

    if (offset >= info_.size)
        return { "BAD OFFSET" };

    std::string buffer(length + 1, '\0');
    file_.seek(offset);

    auto result = file_.read(&buffer[0], length);
    if (result.is_ok())
        ;
    else
        return result.error().what();
    
    return buffer;
}

void TextEditorView::paint_text(Painter& painter, uint32_t line, uint16_t col) {
    // TODO: A line cache would use more memory but save a lot of IO.
    // Only the new lines/characters would need to be refetched.

    auto r = screen_rect();
    auto& lines = info_.newlines;
    auto line_offset = line == 0 ? 0 : (lines[line] + 1);

    // Clean up old text. TODO: smarter invalidation.
    painter.fill_rectangle({ 8 * max_col, r.location().y(), 8, 16 * 16 }, style_default.background);
    painter.fill_rectangle({ 0, r.location().y() + 16 * max_line, 8 * 30, 16 }, style_default.background);

    for (uint32_t i = 0; i <= max_line && i < lines.size(); ++i) {
        auto line_end = lines[line + i];
        auto read_length = max_col;
        if (col + read_length >= line_end)
            read_length = line_end - col;

        if (read_length > 0) {
            painter.draw_string(
                { 0, r.location().y() + (int)i * 16 },
                style_default, read(line_offset + col, read_length));
        }

        line_offset = lines[line + i] + 1;
    }
}

void TextEditorView::paint_cursor(Painter& painter) {
    auto draw_cursor = [this, &painter](uint32_t line, uint16_t col, Color c) {
        auto r = screen_rect();
        line = std::min<uint32_t>(line, max_line);
        col = std::min<uint16_t>(col, max_col);

        painter.draw_rectangle(
            { (int)col * 8, r.location().y() + (int)line * 16, 8, 16 }, c);
    };

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