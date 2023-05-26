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
#include "ui_font_fixed_5x8.hpp"
#include "ui_navigation.hpp"
#include "ui_painter.hpp"
#include "ui_widget.hpp"

#include "circular_buffer.hpp"
#include "file.hpp"
#include "optional.hpp"

#include <memory>
#include <string>
#include <vector>

namespace ui {

/* TODO:
 * - Copy on write into temp file so startup is fast.
 */

enum class LineEnding : uint8_t {
    LF,
    CRLF
};

enum class ScrollDirection : uint8_t {
    Vertical,
    Horizontal
};

/* Wraps a file and provides an API for accessing lines efficiently. */
class FileWrapper {
   public:
    using Error = std::filesystem::filesystem_error;
    using Offset = uint32_t;  // TODO: make enums?
    using Line = uint32_t;
    using Column = uint32_t;
    using Range = struct {
        // Offset of the line start.
        Offset start;
        // Offset of one past the line end.
        Offset end;
    };

    FileWrapper();

    /* Prevent copies. */
    FileWrapper(const FileWrapper&) = delete;
    FileWrapper& operator=(const FileWrapper&) = delete;

    Optional<Error> open(const std::filesystem::path& path);
    std::string get_text(Line line, Column col, Offset length);

    File::Size size() const { return file_.size(); }
    uint32_t line_count() const { return line_count_; }

    Optional<Range> line_range(Line line);
    Offset line_length(Line line);

   private:
    /* Number of newline offsets to cache. */
    static constexpr Offset max_newlines = 64;
    static constexpr size_t buffer_size = 512;

    void initialize();
    std::string read(Offset offset, Offset length = 30);

    /* Returns the offset into the newline cache if valid. */
    Optional<Offset> offset_for_line(Line line) const;

    /* Ensure specified line is in the newline cache. */
    void ensure_cached(Line line);

    /* Helpers for finding the prev/next newline. */
    Optional<Offset> previous_newline(Offset start);
    Optional<Offset> next_newline(Offset start);

    File file_{};

    /* Total number of lines in the file. */
    Offset line_count_{0};

    /* The offset and line of the newlines cache. */
    Offset start_offset_{0};
    Offset start_line_{0};

    LineEnding line_ending_{LineEnding::LF};
    CircularBuffer<Offset, max_newlines + 1> newlines_{};
};

/* Control that renders a text file. */
class TextViewer : public Widget {
   public:
    TextViewer(Rect parent_rect);

    TextViewer(const TextViewer&) = delete;
    TextViewer(TextViewer&&) = delete;
    TextViewer& operator=(const TextViewer&) = delete;
    TextViewer& operator=(TextViewer&&) = delete;

    std::function<void()> on_select{};
    std::function<void()> on_cursor_moved{};

    void paint(Painter& painter) override;
    bool on_key(KeyEvent delta) override;
    bool on_encoder(EncoderEvent delta) override;

    void redraw(bool redraw_text = false);

    void set_file(FileWrapper& file) { reset_file(&file); }
    void clear_file() { reset_file(); }
    bool has_file() const { return file_ != nullptr; }

    uint32_t line() const { return cursor_.line; }
    uint32_t col() const { return cursor_.col; }

   private:
    static constexpr int8_t char_width = 5;
    static constexpr int8_t char_height = 8;
    static constexpr Style style_text{
        .font = font::fixed_5x8,
        .background = Color::black(),
        .foreground = Color::white(),
    };

    const uint8_t max_line = 32;
    const uint8_t max_col = 48;

    /* Returns true if the cursor was updated. */
    bool apply_scrolling_constraints(
        int16_t delta_line,
        int16_t delta_col);

    void paint_text(Painter& painter, uint32_t line, uint16_t col);
    void paint_cursor(Painter& painter);

    void reset_file(FileWrapper* file = nullptr);

    // Gets the length of the current line.
    uint16_t line_length();

    FileWrapper* file_{};

    struct {
        // Previous cursor state.
        uint32_t line{};
        uint16_t col{};

        // Previous draw state.
        uint32_t first_line{};
        uint16_t first_col{};
        bool redraw_text{true};
    } paint_state_{};

    struct {
        uint32_t line{};
        uint16_t col{};
        ScrollDirection dir{ScrollDirection::Vertical};
    } cursor_{};
};

/* Menu control for the TextEditor. */
class TextEditorMenu : public View {
   public:
    TextEditorMenu();

    void on_show() override;
    void on_hide() override;

    std::function<void()>& on_cut() { return button_cut.on_select; }
    std::function<void()>& on_paste() { return button_paste.on_select; }
    std::function<void()>& on_copy() { return button_copy.on_select; }

    std::function<void()>& on_delete_line() { return button_delline.on_select; }
    std::function<void()>& on_edit_line() { return button_edit.on_select; }
    std::function<void()>& on_add_line() { return button_addline.on_select; }

    std::function<void()>& on_open() { return button_open.on_select; }
    std::function<void()>& on_save() { return button_save.on_select; }
    std::function<void()>& on_exit() { return button_exit.on_select; }

   private:
    void hide_children(bool hidden);

    Rectangle rect_frame{
        {0 * 8, 0 * 8, 23 * 8, 23 * 8},
        Color::dark_grey()};

    NewButton button_cut{
        {1 * 8, 1 * 8, 7 * 8, 7 * 8},
        "Cut",
        &bitmap_icon_cut,
        Color::dark_grey()};

    NewButton button_paste{
        {8 * 8, 1 * 8, 7 * 8, 7 * 8},
        "Paste",
        &bitmap_icon_paste,
        Color::dark_grey()};

    NewButton button_copy{
        {15 * 8, 1 * 8, 7 * 8, 7 * 8},
        "Copy",
        &bitmap_icon_copy,
        Color::dark_grey()};

    NewButton button_delline{
        {1 * 8, 8 * 8, 7 * 8, 7 * 8},
        "-Line",
        &bitmap_icon_delete,
        Color::dark_red()};

    NewButton button_edit{
        {8 * 8, 8 * 8, 7 * 8, 7 * 8},
        "Edit",
        &bitmap_icon_rename,
        Color::dark_blue()};

    NewButton button_addline{
        {15 * 8, 8 * 8, 7 * 8, 7 * 8},
        "+Line",
        &bitmap_icon_scanner,
        Color::dark_blue()};

    NewButton button_open{
        {1 * 8, 15 * 8, 7 * 8, 7 * 8},
        "Open",
        &bitmap_icon_load,
        Color::green()};

    NewButton button_save{
        {8 * 8, 15 * 8, 7 * 8, 7 * 8},
        "Save",
        &bitmap_icon_save,
        Color::green()};

    NewButton button_exit{
        {15 * 8, 15 * 8, 7 * 8, 7 * 8},
        "Exit",
        &bitmap_icon_previous,
        Color::dark_red()};
};

/* View viewing and minor edits on a text file. */
class TextEditorView : public View {
   public:
    TextEditorView(NavigationView& nav);
    TextEditorView(
        NavigationView& nav,
        const std::filesystem::path& path);

    std::string title() const override {
        return "Notepad";
    };

    void on_show() override;

   private:
    void open_file(const std::filesystem::path& path);
    void refresh_ui();
    void update_position();
    void hide_menu(bool hidden = true);
    void show_file_picker();
    void show_nyi();

    NavigationView& nav_;
    std::unique_ptr<FileWrapper> file_{};

    TextViewer viewer{
        /* 272 = 320 - 16 (top bar) - 32 (bottom controls) */
        {0, 0, 240, 272}};

    TextEditorMenu menu{};

    NewButton button_menu{
        {26 * 8, 34 * 8, 4 * 8, 4 * 8},
        {},
        &bitmap_icon_controls,
        Color::dark_grey()};

    Text text_position{
        {0 * 8, 34 * 8, 26 * 8, 2 * 8},
        ""};

    Text text_size{
        {0 * 8, 36 * 8, 26 * 8, 2 * 8},
        ""};
};

}  // namespace ui

#endif  // __UI_TEXT_EDITOR_H__