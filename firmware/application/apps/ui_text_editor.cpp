/*
 * Copyright (C) 2023 Kyle Reed
 * Copyright (C) 2023 Mark Thompson
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

namespace ui {

/* TextViewer *******************************************************/

TextViewer::TextViewer(Rect parent_rect)
    : Widget(parent_rect) {
    set_focusable(true);
    set_font_zoom(false);
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
        paint_state_.line = UINT32_MAX;  // forget old cursor position when overwritten
    }

    if (paint_state_.redraw_text) {
        paint_text(painter, first_line, first_col);
        paint_state_.redraw_text = false;
    }

    if (paint_state_.redraw_marked) {
        paint_marked(painter);
        paint_state_.redraw_marked = false;
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
    else {
        delta *= 16;
        updated = apply_scrolling_constraints(delta, 0);
    }

    if (updated)
        redraw();

    return updated;
}

void TextViewer::redraw(bool redraw_text, bool redraw_marked) {
    paint_state_.redraw_text = redraw_text;
    paint_state_.redraw_marked = redraw_marked;
    set_dirty();
}

uint32_t TextViewer::offset() const {
    auto range = file_->line_range(cursor_.line);
    if (range)
        return range->start + col();
    return 0;
}

void TextViewer::cursor_home() {
    cursor_.col = 0;
    redraw();
}

void TextViewer::cursor_end() {
    cursor_.col = line_length() - 1;
    redraw();
}

void TextViewer::cursor_set(uint16_t line, uint16_t col) {
    cursor_.line = line;
    cursor_.col = col;
}

void TextViewer::cursor_mark_selected() {
    LineColPair newMarker = std::make_pair(cursor_.line, cursor_.col);
    auto it = std::find(lineColPair.begin(), lineColPair.end(), newMarker);

    if (it != lineColPair.end()) {
        lineColPair.erase(it);
    } else {
        lineColPair.push_back(newMarker);
    }

    // Mark pending change.
    cursor_.mark_change = false;

    redraw();
}

void TextViewer::cursor_clear_marked() {
    lineColPair.clear();
    redraw(true, true);
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

    // Snap to first/last line to make navigating easier.
    if (new_line < 0 && cursor_.line > 0) {
        new_line = 0;
    } else if (new_line >= (int32_t)file_->line_count()) {
        auto last_line = file_->line_count() - 1;

        if (cursor_.line < last_line)
            new_line = last_line;
    }

    if (new_line < 0 || (uint32_t)new_line >= file_->line_count())
        return false;

    new_line_length = file_->line_length(new_line);

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
                style(), {buffer, *result});

        // Clear empty line sections. This is less visually jarring than full clear.
        int32_t clear_width = max_col - (result ? *result : 0);
        if (clear_width > 0)
            painter.fill_rectangle(
                {(max_col - clear_width) * char_width,
                 r.top() + (int)i * char_height,
                 clear_width * char_width, char_height},
                style().background);

        // if cursor line got overwritten, disable XOR of old cursor when displaying new cursor
        if (i == paint_state_.line)
            paint_state_.line = UINT32_MAX;
    }
}

void TextViewer::paint_cursor(Painter& painter) {
    if (!has_focus())
        return;

    auto xor_cursor = [this, &painter](int32_t line, uint16_t col) {
        int cursor_width = char_width + 1;
        int x = (col - paint_state_.first_col) * char_width - 1;
        if (x < 0) {  // cursor is one pixel narrower when in left column
            cursor_width--;
            x = 0;
        }
        int y = screen_rect().top() + (line - paint_state_.first_line) * char_height;

        // Converting one row at a time to reduce buffer size
        auto pbuf8 = cursor_.pixel_buffer8;
        auto pbuf = cursor_.pixel_buffer;
        for (auto col = 0; col < char_height; col++) {
            // Annoyingly, read_pixels uses a 24-bit pixel format vs draw_pixels which uses 16-bit
            portapack::display.read_pixels({x, y + col, cursor_width, 1}, pbuf8, cursor_width);
            for (auto i = 0; i < cursor_width; i++)
                pbuf[i] = Color(pbuf8[i].r, pbuf8[i].g, pbuf8[i].b).v ^ 0xFFFF;
            portapack::display.draw_pixels({x, y + col, cursor_width, 1}, pbuf, cursor_width);
        }
    };

    if (paint_state_.line != UINT32_MAX)  // only XOR old cursor if it still appears on the screen
    {
        // Only reset previous cursor if we aren't marking.
        if (paint_state_.mark_change) {
            xor_cursor(paint_state_.line, paint_state_.col);
        }
    }

    xor_cursor(cursor_.line, cursor_.col);

    paint_state_.line = cursor_.line;
    paint_state_.col = cursor_.col;
    paint_state_.mark_change = cursor_.mark_change;

    // Reset marking and wait for new change.
    cursor_.mark_change = true;
}

void TextViewer::paint_marked(Painter& painter) {
    auto xor_cursor = [this, &painter](int32_t line, uint16_t col) {
        int cursor_width = char_width + 1;
        int x = (col - paint_state_.first_col) * char_width - 1;
        if (x < 0) {  // cursor is one pixel narrower when in left column
            cursor_width--;
            x = 0;
        }
        int y = screen_rect().top() + (line - paint_state_.first_line) * char_height;

        // Converting one row at a time to reduce buffer size
        auto pbuf8 = cursor_.pixel_buffer8;
        auto pbuf = cursor_.pixel_buffer;
        for (auto col = 0; col < char_height; col++) {
            // Annoyingly, read_pixels uses a 24-bit pixel format vs draw_pixels which uses 16-bit
            portapack::display.read_pixels({x, y + col, cursor_width, 1}, pbuf8, cursor_width);
            for (auto i = 0; i < cursor_width; i++)
                pbuf[i] = Color(pbuf8[i].r, pbuf8[i].g, pbuf8[i].b).v ^ 0xFFFF;
            portapack::display.draw_pixels({x, y + col, cursor_width, 1}, pbuf, cursor_width);
        }
    };

    auto it = lineColPair.begin();

    while (it != lineColPair.end()) {
        LineColPair entry = (LineColPair)*it;
        xor_cursor(entry.first, entry.second);
        it++;
    }
}

void TextViewer::reset_file(FileWrapper* file) {
    file_ = file;
    paint_state_.first_line = 0;
    paint_state_.first_col = 0;
    cursor_.line = 0;
    cursor_.col = 0;
    redraw(true);
}

void TextViewer::set_font_zoom(bool zoom) {
    font_zoom = zoom;
    font_style = font_zoom ? Theme::getInstance()->bg_darkest : Theme::getInstance()->bg_darkest_small;
    char_height = style().font.line_height();
    char_width = style().font.char_width();
    max_line = (uint8_t)(parent_rect().height() / char_height);
    max_col = (uint8_t)(parent_rect().width() / char_width);
}

/* TextEditorMenu ***************************************************/

TextEditorMenu::TextEditorMenu()
    : View{{7 * 4, 9 * 4, 25 * 8, 25 * 8}} {
    add_children(
        {
            &rect_frame,
            &button_home,
            &button_end,
            &button_zoom,
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

static fs::path get_temp_path(const fs::path& path) {
    if (!path.empty())
        return path + "~";

    return {};
}

static void delete_temp_file(const fs::path& path) {
    auto temp_path = get_temp_path(path);
    if (!temp_path.empty()) {
        delete_file(temp_path);
    }
}

static void save_temp_file(const fs::path& path) {
    delete_file(path);
    copy_file(get_temp_path(path), path);
}

static void show_save_prompt(
    NavigationView& nav,
    std::function<void()> on_save,
    std::function<void()> continuation) {
    nav.display_modal(
        "Save?", "       Save changes?", YESNO,
        [on_save](bool choice) {
            if (choice && on_save)
                on_save();
        });
    nav.set_on_pop(continuation);
}

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

    text_position.set_style(Theme::getInstance()->option_active);
    text_size.set_style(Theme::getInstance()->option_active);

    viewer.set_font_zoom(enable_zoom);

    viewer.on_select = [this]() {
        // Treat as if menu button was pressed.
        if (button_menu.on_select)
            button_menu.on_select();
    };

    viewer.on_cursor_moved = [this]() {
        update_position();
    };

    menu.hidden(true);
    menu.on_home() = [this]() {
        viewer.cursor_home();
        hide_menu(true);
    };

    menu.on_end() = [this]() {
        viewer.cursor_end();
        hide_menu(true);
    };

    menu.on_zoom() = [this]() {
        enable_zoom = viewer.toggle_font_zoom();
        refresh_ui();
        hide_menu(true);
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
        show_save_prompt([this]() {
            show_file_picker();
        });
    };

    menu.on_save() = [this]() {
        save_temp_file();
        hide_menu(true);
    };

    menu.on_exit() = [this]() {
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

TextEditorView::~TextEditorView() {
    // NB: Be careful here. The UI will render after this instance
    // has been destroyed. Everything needed to render the UI
    // and perform the save actions must be value captured.
    if (file_dirty_) {
        ui::show_save_prompt(
            nav_,
            [p = path_]() { ui::save_temp_file(p); },
            [p = std::move(path_)]() { delete_temp_file(p); });
    }
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
    delete_temp_file(path_);

    path_ = {};
    file_dirty_ = false;
    has_temp_file_ = false;
    auto result = FileWrapper::open(
        path, false, [](uint32_t value, uint32_t total) {
            Painter p;
            auto percent = (value * 100) / total;
            auto width = (percent * screen_width) / 100;
            p.draw_hline({0, 16}, width, Theme::getInstance()->fg_yellow->foreground);
        });

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

void TextEditorView::show_file_picker() {
    auto open_view = nav_.push<FileLoadView>("");
    open_view->on_changed = [this](std::filesystem::path path) {
        // Can't update the UI focus while the FileLoadView is still up.
        // Do this on a continuation instead of in on_changed.
        nav_.set_on_pop([this, p = std::move(path)]() {
            open_file(p);
            hide_menu();
        });
    };
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

void TextEditorView::show_save_prompt(std::function<void()> continuation) {
    if (!file_dirty_) {
        if (continuation)
            continuation();
        return;
    }

    ui::show_save_prompt(
        nav_,
        [this]() { save_temp_file(); },
        continuation);
}

void TextEditorView::prepare_for_write() {
    file_dirty_ = true;

    if (has_temp_file_)
        return;

    // TODO: This would be nice to have but it causes a stack overflow in an ISR?
    // Painter p;
    // p.draw_string({2, 48}, *Theme::getInstance()->fg_yellow, "Creating temporary file...");

    // Copy to temp file on write.
    has_temp_file_ = true;
    delete_temp_file(path_);
    copy_file(path_, get_temp_path(path_));
    file_->assume_file(get_temp_path(path_));
}

void TextEditorView::save_temp_file() {
    if (file_dirty_) {
        ui::save_temp_file(path_);
        file_dirty_ = false;
    }
}

}  // namespace ui
