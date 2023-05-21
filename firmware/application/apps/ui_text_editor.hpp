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

// TODO: find a common place for this.
/* Implements a fixed-size, circular buffer.
 * NB: Holds Capacity - 1 items. */
template <typename T, size_t Capacity>
class CircularBuffer {
   public:
    CircularBuffer() = default;

    CircularBuffer(const CircularBuffer&) = delete;
    CircularBuffer(CircularBuffer&&) = delete;
    CircularBuffer& operator=(const CircularBuffer&) = delete;
    CircularBuffer& operator=(CircularBuffer&&) = delete;

    void push_front(T val) {
        head_ = head_ > 0 ? head_ - 1 : last_index;
        if (head_ == end_)
            pop_back();

        data_[head_] = std::move(val);
    }

    void pop_front() {
        if (!empty())
            head_ = head_ < last_index ? head_ + 1 : 0;
    }

    void push_back(T val) {
        data_[end_] = std::move(val);

        end_ = end_ < last_index ? end_ + 1 : 0;
        if (head_ == end_)
            pop_front();
    }

    void pop_back() {
        if (!empty())
            end_ = end_ > 0 ? end_ - 1 : last_index;
    }

    T& operator[](size_t ix) {
        ix += head_;
        if (ix >= Capacity)
            ix -= Capacity;
        return data_[ix];
    }

    const T& operator[](size_t ix) const {
        return const_cast<CircularBuffer*>(this)->operator[](ix);
    }

    const T& front() const {
        return data_[head_];
    }

    const T& back() const {
        auto end = end_ > 0 ? end_ - 1 : last_index;
        return data_[end];
    }

    size_t size() const {
        auto end = end_;
        if (end < head_)
            end += Capacity;
        return end - head_;
    }

    bool empty() const {
        return head_ == end_;
    }

    void clear() {
        head_ = 0;
        end_ = 0;
    }

   private:
    static constexpr size_t last_index = Capacity;
    size_t head_{0};
    size_t end_{0};

    T data_[Capacity]{};
};

/* Wraps a file and provides an API for accessing lines efficiently. */
class FileWrapper {
   public:
    using Error = std::filesystem::filesystem_error;
    using Offset = uint32_t;  // TODO: make enums?
    using Line = uint32_t;
    using Column = uint32_t;
    using Range = struct {
        Offset start;
        Offset end;
    };

    FileWrapper() = default;

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
    // LogFile  log_{};

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
        {0 * 8, 36 * 8, 24 * 8, 2 * 8},
        ""};
};

}  // namespace ui

#endif  // __UI_TEXT_EDITOR_H__