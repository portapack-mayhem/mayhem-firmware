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

void TextViewer::redraw(bool redraw_text) {
    paint_state_.redraw_text = redraw_text;
    set_dirty();
}

uint32_t TextViewer::offset() const {
    auto range = file_->line_range(cursor_.line);
    if (range)
        return range->start + col();
    return 0;
}

uint16_t TextViewer::line_length() {
    return file_->line_length(cursor_.line);
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

void TextViewer::paint_text(Painter& painter, uint32_t line, uint16_t col) {
    auto r = screen_rect();
    char buffer[max_col + 1];

    // Draw the lines from the file
    for (auto i = 0u; i < max_line; ++i) {
        if (line + i >= file_->line_count())
            break;

        auto result = file_->get_text(line + i, col, buffer, max_col);

        if (result && *result > 0)
            painter.draw_string(
                {0, r.top() + (int)i * char_height},
                Styles::white_small, {buffer, *result});

        // Clear empty line sections. This is less visually jarring than full clear.
        int32_t clear_width = max_col - (result ? *result : 0);
        if (clear_width > 0)
            painter.fill_rectangle(
                {(max_col - clear_width) * char_width,
                 r.top() + (int)i * char_height,
                 clear_width * char_width, char_height},
                Styles::white_small.background);
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
    draw_cursor(paint_state_.line, paint_state_.col, Styles::white_small.background);
    draw_cursor(cursor_.line, cursor_.col, Styles::white_small.foreground);
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
        prepare_for_write();
        file_->delete_line(viewer.line());
        refresh_ui();
        hide_menu(true);
    };

    menu.on_edit_line() = [this]() {
        show_edit_line();
    };

    menu.on_add_line() = [this]() {
        prepare_for_write();

        if (viewer.offset() < file_->size() - 1)
            file_->insert_line(viewer.line());
        else
            file_->insert_line(-1);  // Add after last line.

        refresh_ui();
        hide_menu(true);
    };

    menu.on_open() = [this]() {
        /*show_save_prompt([this]() {
            show_file_picker();
        });*/
        // HACK: above should work but it's faulting.
        if (!file_dirty_) {
            show_file_picker();
        } else {
            show_save_prompt(nullptr);
            show_file_picker(false);
        }
    };

    menu.on_save() = [this]() {
        save_temp_file();
        hide_menu(true);
    };

    menu.on_exit() = [this]() {
        show_save_prompt([this]() {
            nav_.pop();
        });
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

TextEditorView::~TextEditorView() {
    delete_temp_file();
}

void TextEditorView::on_show() {
    if (file_)
        viewer.focus();
    else
        button_menu.focus();
}

void TextEditorView::open_file(const fs::path& path) {
    file_.reset();
    viewer.clear_file();
    delete_temp_file();

    path_ = {};
    file_dirty_ = false;
    has_temp_file_ = false;
    auto result = FileWrapper::open(path);

    if (!result) {
        nav_.display_modal("Read Error", "Cannot open file:\n" + result.error().what());

    } else {
        file_ = *std::move(result);
        path_ = path;
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

void TextEditorView::show_file_picker(bool immediate) {
    // TODO: immediate is a hack until nav_.on_pop is fixed.
    auto open_view = immediate ? nav_.push<FileLoadView>("") : nav_.push_under_current<FileLoadView>("");

    if (open_view) {
        open_view->on_changed = [this](std::filesystem::path path) {
            open_file(path);
            hide_menu();
        };
    }
}

void TextEditorView::show_edit_line() {
    auto str = file_->get_text(viewer.line(), 0, viewer.line_length());
    if (!str) {
        nav_.display_modal("Error", "Failed to get line text.");
        return;
    }

    edit_line_buffer_ = *std::move(str);

    text_prompt(
        nav_,
        edit_line_buffer_,
        viewer.col(),
        max_edit_length,
        [this](std::string& buffer) {
            auto range = file_->line_range(viewer.line());
            if (!range)
                return;

            prepare_for_write();
            file_->replace_range(*range, buffer);
        });
    nav_.set_on_pop([this]() {
        edit_line_buffer_.clear();
        refresh_ui();
        hide_menu(true);
    });
}

void TextEditorView::show_nyi() {
    nav_.display_modal("Soon...", "Coming soon.");
}

void TextEditorView::show_save_prompt(std::function<void()> continuation) {
    if (!file_dirty_) {
        if (continuation)
            continuation();
        return;
    }

    nav_.display_modal(
        "Save?", "Save changes?", YESNO,
        [this](bool choice) {
            if (choice)
                save_temp_file();
        });
    nav_.set_on_pop(continuation);
}

void TextEditorView::prepare_for_write() {
    file_dirty_ = true;

    if (has_temp_file_)
        return;

    // Copy to temp file on write.
    has_temp_file_ = true;
    delete_temp_file();
    copy_file(path_, get_temp_path());
    file_->assume_file(get_temp_path());
}

void TextEditorView::delete_temp_file() const {
    auto temp_path = get_temp_path();
    if (!temp_path.empty()) {
        delete_file(temp_path);
    }
}

void TextEditorView::save_temp_file() {
    if (file_dirty_) {
        delete_file(path_);
        copy_file(get_temp_path(), path_);
        file_dirty_ = false;
    }
}

fs::path TextEditorView::get_temp_path() const {
    if (!path_.empty())
        return path_ + "~";

    return {};
}

}  // namespace ui
