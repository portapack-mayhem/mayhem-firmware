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
#include "ui_textentry.hpp"

#include "log_file.hpp"
#include "string_format.hpp"

using namespace portapack;
namespace fs = std::filesystem;

namespace {
/*void log(const std::string& msg) {
    LogFile log{};
    log.append("LOGS/Notepad.txt");
    log.write_entry(msg);
}*/
}  // namespace

namespace ui {

/* FileWrapper ******************************************************/

FileWrapper::FileWrapper() {
}

Optional<FileWrapper::Error> FileWrapper::open(const fs::path& path) {
    file_ = File();
    auto error = file_.open(path);

    if (!error)
        initialize();

    return error;
}

std::string FileWrapper::get_text(Offset line, Offset col, Offset length) {
    // TODO: better way to return errors.
    auto range = line_range(line);
    int32_t to_read = length;

    if (!range)
        return "[UNCACHED LINE]";

    // Don't read past end of line.
    if (range->start + col + to_read >= range->end)
        to_read = range->end - col - range->start;

    if (to_read <= 0)
        return {};

    return read(range->start + col, to_read);
}

Optional<FileWrapper::Range> FileWrapper::line_range(Line line) {
    ensure_cached(line);

    auto offset = offset_for_line(line);
    if (!offset)
        return {};

    auto start = *offset == 0 ? start_offset_ : (newlines_[*offset - 1] + 1);
    auto end = newlines_[*offset] + 1;

    return {Range{start, end}};
}

FileWrapper::Offset FileWrapper::line_length(Line line) {
    auto range = line_range(line);

    if (range)
        return range->end - range->start;

    return 0;
}

void FileWrapper::initialize() {
    start_offset_ = 0;
    start_line_ = 0;
    line_count_ = 0;
    newlines_.clear();
    line_ending_ = LineEnding::LF;

    Offset offset = 0;
    auto result = next_newline(offset);

    while (result) {
        ++line_count_;
        if (newlines_.size() < max_newlines)
            newlines_.push_back(*result);
        offset = *result + 1;
        result = next_newline(offset);
    }
}

std::string FileWrapper::read(Offset offset, Offset length) {
    // TODO: better way to return errors.
    if (offset + length > file_.size())
        return {"[BAD OFFSET]"};

    std::string buffer(length, '\0');
    file_.seek(offset);

    auto result = file_.read(&buffer[0], length);
    if (result.is_ok())
        buffer.resize(*result);
    else
        return result.error().what();

    return buffer;
}

Optional<FileWrapper::Offset> FileWrapper::offset_for_line(Line line) const {
    if (line >= line_count_)
        return {};

    Offset actual = line - start_line_;
    if (actual < newlines_.size())  // NB: underflow wrap.
        return {actual};

    return {};
}

void FileWrapper::ensure_cached(Line line) {
    if (line >= line_count_)
        return;

    auto result = offset_for_line(line);
    if (result)
        return;

    if (line < start_line_) {
        while (line < start_line_ && start_offset_ >= 2) {
            // start_offset_ - 1 should be a newline. Need to
            // find the new value for start_offset_. start_line_
            // has to be > 0 to get into this block so there should
            // always be one newline before start_offset_.
            auto offset = previous_newline(start_offset_ - 2);
            newlines_.push_front(start_offset_ - 1);

            if (!offset) {
                // Must be at beginning.
                start_line_ = 0;
                start_offset_ = 0;
            } else {
                // Found an previous newline, the new start_line_
                // starts at the newline offset + 1.
                start_line_--;
                start_offset_ = *offset + 1;
            }
        }
    } else {
        while (line >= start_line_ + newlines_.size()) {
            auto offset = next_newline(newlines_.back() + 1);
            if (offset) {
                start_line_++;
                start_offset_ = newlines_.front() + 1;
                newlines_.push_back(*offset);
            } /* else at the EOF. */
        }
    }
}

Optional<FileWrapper::Offset> FileWrapper::previous_newline(Offset start) {
    char buffer[buffer_size];
    Offset offset = start;
    auto to_read = buffer_size;

    do {
        if (offset < to_read) {
            // NB: Char at 'offset' was read in the previous iteration.
            to_read = offset;
            offset = 0;
        } else
            offset -= to_read;

        file_.seek(offset);

        auto result = file_.read(buffer, to_read);
        if (result.is_error())
            break;

        // Find newlines in the buffer backwards.
        for (int32_t i = *result - 1; i >= 0; --i) {
            switch (buffer[i]) {
                case '\n':
                    return {offset + i};
            }
        }

        if (offset == 0)
            break;

    } while (true);

    return {};  // Didn't find one.
}

Optional<FileWrapper::Offset> FileWrapper::next_newline(Offset start) {
    char buffer[buffer_size];
    Offset offset = start;

    // EOF, nothing to do.
    if (start >= size())
        return {};

    file_.seek(offset);

    while (true) {
        auto result = file_.read(buffer, buffer_size);
        if (result.is_error())
            return {};

        // Find newlines in the buffer.
        for (Offset i = 0; i < *result; ++i) {
            switch (buffer[i]) {
                case '\n':
                    return {offset + i};
            }
        }

        offset += *result;

        if (*result < buffer_size)
            break;
    }

    // Fake a newline at the end for consistency.
    return {offset};
}

/* TextViewer *******************************************************/

TextViewer::TextViewer(Rect parent_rect)
    : Widget(parent_rect),
      max_line{static_cast<uint8_t>(parent_rect.height() / char_height)},
      max_col{static_cast<uint8_t>(parent_rect.width() / char_width)} {
    set_focusable(true);
}

void TextViewer::paint(Painter& painter) {
    auto first_line = paint_state_.first_line;
    auto first_col = paint_state_.first_col;

    if (!has_file())
        return;

    // Move the viewport vertically.
    if (cursor_.line < first_line)
        first_line = cursor_.line;
    else if (cursor_.line >= first_line + max_line)
        first_line = cursor_.line - max_line + 1;

    // Move the viewport horizontally.
    if (cursor_.col < first_col)
        first_col = cursor_.col;
    if (cursor_.col >= first_col + max_col)
        first_col = cursor_.col - max_col + 1;

    // Viewport updated? Redraw text.
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

bool TextViewer::on_key(const KeyEvent key) {
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
    else if (key == KeyEvent::Select && on_select) {
        on_select();
        return true;
    }

    // Always allow cursor direction to be updated.
    cursor_.dir = delta_col != 0 ? ScrollDirection::Horizontal : ScrollDirection::Vertical;
    auto updated = apply_scrolling_constraints(delta_line, delta_col);

    if (updated)
        redraw();

    return updated;
}

bool TextViewer::on_encoder(EncoderEvent delta) {
    bool updated = false;

    if (cursor_.dir == ScrollDirection::Horizontal)
        updated = apply_scrolling_constraints(0, delta);
    else
        updated = apply_scrolling_constraints(delta, 0);

    if (updated)
        redraw();

    return updated;
}

bool TextViewer::apply_scrolling_constraints(int16_t delta_line, int16_t delta_col) {
    if (!has_file())
        return false;

    int32_t new_line = cursor_.line + delta_line;
    int32_t new_col = cursor_.col + delta_col;
    int32_t new_line_length = file_->line_length(new_line);

    if (new_col < 0)
        --new_line;
    else if (new_col >= new_line_length && delta_line == 0) {
        // Only wrap if moving horizontally.
        new_col = 0;
        ++new_line;
    }

    // Snap to first/last to make navigating easier.
    if (new_line < 0 && new_col > 0) {
        new_line = 0;
        new_col = 0;
    } else if (new_line >= (int32_t)file_->line_count()) {
        auto last_line = file_->line_count() - 1;
        int32_t last_col = file_->line_length(last_line) - 1;

        if (new_col < last_col) {
            new_line = last_line;
            new_col = last_col;
        }
    }

    if (new_line < 0 || (uint32_t)new_line >= file_->line_count())
        return false;

    new_line_length = file_->line_length(new_line);

    // TODO: don't wrap with encoder?
    // Wrap or clamp column.
    if (new_line_length == 0)
        new_col = 0;
    else if (new_col >= new_line_length || new_col < 0)
        new_col = new_line_length - 1;

    cursor_.line = new_line;
    cursor_.col = new_col;

    if (on_cursor_moved)
        on_cursor_moved();

    return true;
}

void TextViewer::redraw(bool redraw_text) {
    paint_state_.redraw_text = redraw_text;
    set_dirty();
}

void TextViewer::paint_text(Painter& painter, uint32_t line, uint16_t col) {
    // CONSIDER: A line cache would use more memory but save a lot of IO.
    // Only the new lines/characters would need to be refetched.

    auto r = screen_rect();

    // Draw the lines from the file
    for (auto i = 0u; i < max_line; ++i) {
        if (line + i >= file_->line_count())
            break;

        auto str = file_->get_text(line + i, col, max_col);

        // Draw text.
        if (str.length() > 0)
            painter.draw_string(
                {0, r.top() + (int)i * char_height},
                style_text, str);

        // Clear empty line sections.
        int32_t clear_width = max_col - str.length();
        if (clear_width > 0)
            painter.fill_rectangle(
                {(max_col - clear_width) * char_width,
                 r.top() + (int)i * char_height,
                 clear_width * char_width, char_height},
                style_text.background);
    }
}

void TextViewer::paint_cursor(Painter& painter) {
    if (!has_focus())
        return;

    auto draw_cursor = [this, &painter](uint32_t line, uint16_t col, Color c) {
        auto r = screen_rect();
        line = line - paint_state_.first_line;
        col = col - paint_state_.first_col;

        painter.draw_rectangle(
            {(int)col * char_width - 1,
             r.top() + (int)line * char_height,
             char_width + 1, char_height},
            c);
    };

    // Clear old cursor. CONSIDER: XOR cursor?
    draw_cursor(paint_state_.line, paint_state_.col, style_text.background);
    draw_cursor(cursor_.line, cursor_.col, style_text.foreground);
    paint_state_.line = cursor_.line;
    paint_state_.col = cursor_.col;
}

void TextViewer::reset_file(FileWrapper* file) {
    file_ = file;
    paint_state_.first_line = 0;
    paint_state_.first_col = 0;
    cursor_.line = 0;
    cursor_.col = 0;
    redraw(true);
}

uint16_t TextViewer::line_length() {
    return file_->line_length(cursor_.line);
}

/* TextEditorMenu ***************************************************/

TextEditorMenu::TextEditorMenu()
    : View{{7 * 4, 9 * 4, 25 * 8, 25 * 8}} {
    add_children(
        {
            &rect_frame,
            &button_cut,
            &button_paste,
            &button_copy,
            &button_delline,
            &button_edit,
            &button_addline,
            &button_open,
            &button_save,
            &button_exit,
        });
}

void TextEditorMenu::on_show() {
    hide_children(false);
    button_edit.focus();
}

void TextEditorMenu::on_hide() {
    hide_children(true);
}

void TextEditorMenu::hide_children(bool hidden) {
    for (auto child : children()) {
        child->hidden(hidden);
    }
}

/* TextEditorView ***************************************************/

TextEditorView::TextEditorView(NavigationView& nav)
    : nav_{nav} {
    add_children(
        {
            &viewer,
            &menu,
            &button_menu,
            &text_position,
            &text_size,
        });

    viewer.on_select = [this]() {
        // Treat as if menu button was pressed.
        if (button_menu.on_select)
            button_menu.on_select();
    };

    viewer.on_cursor_moved = [this]() {
        update_position();
    };

    menu.hidden(true);
    menu.on_cut() = [this]() {
        show_nyi();
    };
    menu.on_paste() = [this]() {
        show_nyi();
    };
    menu.on_copy() = [this]() {
        show_nyi();
    };
    menu.on_delete_line() = [this]() {
        show_nyi();
    };
    menu.on_edit_line() = [this]() {
        show_nyi();
    };
    menu.on_add_line() = [this]() {
        show_nyi();
    };
    menu.on_open() = [this]() {
        // TODO: confirm.
        show_file_picker();
    };
    menu.on_save() = [this]() {
        show_nyi();
    };
    menu.on_exit() = [this]() {
        // TODO: confirm.
        nav_.pop();
    };

    button_menu.on_select = [this]() {
        if (file_) {
            // Toggle menu.
            hide_menu(!menu.hidden());
        } else {
            show_file_picker();
        }
    };
}

TextEditorView::TextEditorView(NavigationView& nav, const fs::path& path)
    : TextEditorView(nav) {
    open_file(path);
}

void TextEditorView::on_show() {
    if (file_)
        viewer.focus();
    else
        button_menu.focus();
}

void TextEditorView::open_file(const fs::path& path) {
    auto file = std::make_unique<FileWrapper>();
    auto error = file->open(path);

    if (error) {
        nav_.display_modal("Read Error", "Cannot open file:\n" + error->what());
        file_.reset();
        viewer.clear_file();
    } else {
        file_ = std::move(file);
        viewer.set_file(*file_);
    }

    refresh_ui();
}

void TextEditorView::refresh_ui() {
    if (file_) {
        update_position();
        text_size.set(
            "Lines:" + to_string_dec_uint(file_->line_count()) +
            " (" + to_string_file_size(file_->size()) + ")");
    } else {
        text_position.set("");
        text_size.set("");
    }
}

void TextEditorView::update_position() {
    if (viewer.has_file()) {
        text_position.set(
            "Ln " + to_string_dec_uint(viewer.line() + 1) +
            ", Col " + to_string_dec_uint(viewer.col() + 1));
    }
}

void TextEditorView::hide_menu(bool hidden) {
    menu.hidden(hidden);

    // Only let the viewer be focused when the menu is
    // not shown, otherwise menu focus gets confusing.
    viewer.set_focusable(hidden);

    if (hidden)
        viewer.focus();

    viewer.redraw(true);
    set_dirty();
}

void TextEditorView::show_file_picker() {
    auto open_view = nav_.push<FileLoadView>("");
    open_view->on_changed = [this](std::filesystem::path path) {
        open_file(path);
        hide_menu();
    };
}

void TextEditorView::show_nyi() {
    nav_.display_modal("Soon...", "Coming soon.");
}

}  // namespace ui