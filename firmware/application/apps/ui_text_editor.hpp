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
//#include "ui_textentry.hpp"

#include "circular_buffer.hpp"
#include "file.hpp"
#include "optional.hpp"

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

class TextEditorView : public View {
   public:
    TextEditorView(NavigationView& nav);
    TextEditorView(
        NavigationView& nav,
        const std::filesystem::path& path);

    std::string title() const override {
        return "Notepad";
    };

    void on_focus() override;
    void paint(Painter& painter) override;
    bool on_key(KeyEvent delta) override;
    bool on_encoder(EncoderEvent delta) override;

   private:
    static constexpr uint8_t max_line = 32;
    static constexpr uint8_t max_col = 48;
    static constexpr int8_t char_width = 5;
    static constexpr int8_t char_height = 8;

    static constexpr Style style_default{
        .font = font::fixed_5x8,
        .background = Color::black(),
        .foreground = Color::white(),
    };

    /* Returns true if the cursor was updated. */
    bool apply_scrolling_constraints(
        int16_t delta_line,
        int16_t delta_col);

    void refresh_ui();
    void open_file(const std::filesystem::path& path);

    void paint_text(Painter& painter, uint32_t line, uint16_t col);
    void paint_cursor(Painter& painter);

    // Gets the length of the current line.
    uint16_t line_length();

    NavigationView& nav_;

    FileWrapper file_{};

    struct {
        // Previous cursor state.
        uint32_t line{};
        uint16_t col{};

        // Previous draw state.
        uint32_t first_line{};
        uint16_t first_col{};
        bool redraw_text{true};
        bool has_file{false};
    } paint_state_{};

    struct {
        uint32_t line{};
        uint16_t col{};
        ScrollDirection dir{ScrollDirection::Vertical};
    } cursor_{};

    // TODO: The scrollable view should be its own widget
    // otherwise control navigation doesn't work.

    Button button_open{
        {24 * 8, 34 * 8, 6 * 8, 4 * 8},
        "Open"};

    Text text_position{
        {0 * 8, 34 * 8, 24 * 8, 2 * 8},
        ""};

    Text text_size{
        {0 * 8, 36 * 8, 24 * 8, 2 * 8},
        ""};
};

}  // namespace ui

#endif  // __UI_TEXT_EDITOR_H__