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

#ifndef __FILE_WRAPPER_HPP__
#define __FILE_WRAPPER_HPP__

#include "circular_buffer.hpp"
#include "file.hpp"
#include "optional.hpp"

#include <functional>
#include <memory>
#include <string_view>

enum class LineEnding : uint8_t {
    LF,
    CRLF
};

/* TODO:
 * - CRLF handling.
 * - Avoid full re-read on edits.
 *   - Would need to read old/new text when editing to track newlines.
 * - How to surface errors? Exceptions?
 */

/* FatFs docs http://elm-chan.org/fsw/ff/00index_e.html */

/* BufferType requires the following members
 * Size size()
 * Result<Size> read(void* data, Size bytes_to_read)
 * Result<Size> write(const void* data, Size bytes_to_write)
 * Result<Offset> seek(uint32_t offset)
 * Result<Offset> truncate()
 * Optional<Error> sync()
 */

/* Wraps a buffer and provides an API for accessing lines efficiently. */
template <typename BufferType, uint32_t CacheSize>
class BufferWrapper {
   public:
    using Offset = uint32_t;
    using Line = uint32_t;
    using Column = uint32_t;
    using Size = File::Size;
    using Range = struct {
        // Offset of the start, inclusive.
        Offset start;
        // Offset of the end, exclusive.
        Offset end;

        Offset length() const { return end - start; }
    };

    BufferWrapper(BufferType* buffer)
        : wrapped_{buffer} {
        initialize();
    }
    virtual ~BufferWrapper() {}

    std::function<void(Size, Size)> on_read_progress{};

    /* Prevent copies */
    BufferWrapper(const BufferWrapper&) = delete;
    BufferWrapper& operator=(const BufferWrapper&) = delete;

    Optional<std::string> get_text(Line line, Column col, Offset length) {
        std::string buffer;
        buffer.resize(length);

        auto result = get_text(line, col, &buffer[0], length);
        if (!result)
            return {};

        buffer.resize(*result);
        return buffer;
    }

    Optional<Offset> get_text(Line line, Column col, char* output, Offset length) {
        auto range = line_range(line);
        int32_t to_read = length;

        if (!range)
            return {};

        // Don't read past end of line.
        if (range->start + col + to_read >= range->end)
            to_read = range->end - col - range->start;

        if (to_read <= 0)
            return {};

        return read(range->start + col, output, to_read);
    }

    /* Gets the size of the buffer in bytes. */
    Size size() const { return wrapped_->size(); }

    /* Get the count of the lines in the buffer. */
    uint32_t line_count() const { return line_count_; }

    /* Gets the range of the line if valid. */
    Optional<Range> line_range(Line line) {
        ensure_cached(line);

        auto index = index_for_line(line);
        if (!index)
            return {};

        auto start = *index == 0 ? start_offset_ : (newlines_[*index - 1] + 1);
        auto end = newlines_[*index] + 1;

        return Range{start, end};
    }

    /* Gets the length of the line, or 0 if invalid. */
    Offset line_length(Line line) {
        auto range = line_range(line);
        return range ? range->length() : 0;
    }

    /* Gets the buffer offset of the line & col if valid. */
    Optional<Offset> get_offset(Line line, Column col) {
        auto range = line_range(line);

        if (range)
            return range->start + col;

        return {};
    }

    /* Gets the index of the first line in the cache.
     * Only really useful for unit testing or diagnostics. */
    Offset start_line() { return start_line_; };

    /* Inserts a line before the specified line or at the
     * end of the buffer if line >= line_count. */
    void insert_line(Line line) {
        auto range = line_range(line);

        if (range)
            replace_range({range->start, range->start}, "\n");
        else if (line >= line_count_)
            replace_range({(Offset)size(), (Offset)size()}, "\n");
    }

    /* Deletes the specified line. */
    void delete_line(Line line) {
        auto range = line_range(line);

        if (range)
            replace_range(*range, {});
    }

    /* Replace the specified range with the string contents.
     * A range with start/end set to the same value will insert.
     * A range with an empty string will delete. */
    void replace_range(Range range, std::string_view value) {
        if (range.start > size() || range.end > size() || range.start > range.end)
            return;

        /* If delta_length == 0, it's an overwrite. Could still have
         * added or removed newlines so caches will need to be rebuilt.
         * If delta_length > 0, the file needs to grow and content needs
         * to be shifted forward until the end of the range.
         * If delta_length < 0, the file needs to be truncated and the
         * content after the value needs to be shifted backward. */
        int32_t delta_length = value.length() - range.length();
        if (delta_length > 0)
            expand(range.end, delta_length);
        else if (delta_length < 0)
            shrink(range.end, delta_length);

        write(range.start, value);
        wrapped_->sync();
        rebuild_cache();
    }

   protected:
    BufferWrapper() {}

    void set_buffer(BufferType* buffer) {
        wrapped_ = buffer;
        initialize();
    }

   private:
    /* Number of newline offsets to cache. */
    static constexpr Offset max_newlines = CacheSize;

    /* Size of stack buffer used for reading/writing. */
    static constexpr Offset buffer_size = 512;

    void initialize() {
        start_offset_ = 0;
        start_line_ = 0;
        line_count_ = 0;
        rebuild_cache();
    }

    void rebuild_cache() {
        newlines_.clear();

        // Special case for empty files to keep them consistent.
        if (size() == 0) {
            line_count_ = 1;
            newlines_.push_back(0);
            return;
        }

        // TODO: think through this for edit cases.
        // E.g. don't read to end, maybe could specify
        // a range to re-read because it should be possible
        // to tell where the dirty regions are. After the
        // dirty region, it should be possible to fixup
        // the line_count data.
        // TODO: seems like shrink/expand could do this while
        // they are running.

        line_count_ = start_line_;
        Offset offset = start_offset_;

        // Report progress every N lines.
        constexpr auto report_interval = 100u;
        auto result = next_newline(offset);
        auto next_report = report_interval;

        while (result) {
            ++line_count_;
            if (newlines_.size() < max_newlines)
                newlines_.push_back(*result);
            offset = *result + 1;

            if (on_read_progress && line_count_ > next_report) {
                on_read_progress(offset, size());
                next_report = line_count_ + report_interval;
            }

            result = next_newline(offset);
        }
    }

    Optional<Offset> read(Offset offset, char* buffer, Offset length) {
        if (offset + length > size())
            return {};

        wrapped_->seek(offset);

        auto result = wrapped_->read(buffer, length);
        if (result.is_error())
            return {};

        return *result;
    }

    bool write(Offset offset, std::string_view value) {
        wrapped_->seek(offset);
        auto result = wrapped_->write(value.data(), value.length());

        return result.is_ok();
    }

    /* Returns the index of the line in the newline cache if valid. */
    Optional<Offset> index_for_line(Line line) const {
        if (line >= line_count_)
            return {};

        Offset actual = line - start_line_;
        if (actual >= newlines_.size())  // NB: underflow wrap.
            return {};

        return actual;
    }

    /* Ensure specified line is in the newline cache. */
    void ensure_cached(Line line) {
        if (line >= line_count_)
            return;

        auto index = index_for_line(line);
        if (index)
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

    /* Finding the first newline backward from offset. */
    Optional<Offset> previous_newline(Offset offset) {
        char buffer[buffer_size];
        auto to_read = buffer_size;

        do {
            if (offset < to_read) {
                // NB: Char at 'offset' was read in the previous iteration.
                to_read = offset;
                offset = 0;
            } else
                offset -= to_read;

            wrapped_->seek(offset);

            auto result = wrapped_->read(buffer, to_read);
            if (result.is_error())
                break;

            // Find newlines in the buffer backwards.
            for (int32_t i = *result - 1; i >= 0; --i) {
                switch (buffer[i]) {
                    case '\n':
                        return offset + i;
                }
            }

            if (offset == 0)
                break;

        } while (true);

        return {};  // Didn't find one.
    }

    /* Finding the first newline forward from offset. */
    Optional<Offset> next_newline(Offset offset) {
        // EOF, no more newlines to find.
        if (offset >= size())
            return {};

        char buffer[buffer_size];
        wrapped_->seek(offset);

        while (true) {
            auto result = wrapped_->read(buffer, buffer_size);
            if (result.is_error())
                return {};

            // Find newlines in the buffer.
            for (Offset i = 0; i < *result; ++i) {
                switch (buffer[i]) {
                    case '\n':
                        return offset + i;
                }
            }

            offset += *result;

            if (*result < buffer_size)
                break;
        }

        // For consistency, treat the end of the file as a "newline".
        return size() - 1;
    }

    /* Grow the file and move file content so that the
     * content at src is shifted forward by 'delta'. */
    void expand(Offset src, int32_t delta) {
        if (delta <= 0)  // Not an expand.
            return;

        char buffer[buffer_size];
        auto to_read = buffer_size;

        // Number of bytes left to shift.
        Offset remaining = size() - src;
        Offset offset = size();
        Size report_total = remaining;
        Size report_interval = report_total / 8;
        Size next_report = remaining - report_interval;

        while (remaining > 0) {
            offset -= std::min(remaining, buffer_size);
            to_read = std::min(remaining, buffer_size);

            wrapped_->seek(offset);
            auto result = wrapped_->read(buffer, to_read);
            if (result.is_error())
                break;

            wrapped_->seek(offset + delta);
            result = wrapped_->write(buffer, *result);
            if (result.is_error())
                break;

            remaining -= *result;

            if (on_read_progress && remaining <= next_report) {
                on_read_progress(report_total - remaining, report_total);
                next_report = remaining > report_interval ? remaining - report_interval : 0;
            }
        }
    }

    /* Shrink the file and move file content so that the
     * content at src is shifted backward by 'delta'. */
    void shrink(Offset src, int32_t delta) {
        if (delta >= 0)  // Not a shrink.
            return;

        char buffer[buffer_size];
        auto offset = src;
        Size report_total = size();
        Size report_interval = report_total / 8;
        Size next_report = offset + report_interval;

        while (true) {
            wrapped_->seek(offset);
            auto result = wrapped_->read(buffer, buffer_size);
            if (result.is_error())
                break;

            wrapped_->seek(offset + delta);
            result = wrapped_->write(buffer, *result);

            if (result.is_error() || *result < buffer_size)
                break;

            offset += *result;

            if (on_read_progress && offset >= next_report) {
                on_read_progress(offset, report_total);
                next_report = offset + report_interval;
            }
        }

        // Delete the extra bytes at the end of the file.
        wrapped_->truncate();
    }

    BufferType* wrapped_{};

    /* Total number of lines in the buffer. */
    Offset line_count_{0};

    /* The offset and line of the newlines cache. */
    Offset start_offset_{0};
    Offset start_line_{0};

    LineEnding line_ending_{LineEnding::LF};
    CircularBuffer<Offset, max_newlines + 1> newlines_{};
};

/* A BufferWrapper over a file. */
class FileWrapper : public BufferWrapper<File, 64> {
   public:
    template <typename T>
    using Result = File::Result<T>;
    using Error = File::Error;
    static Result<std::unique_ptr<FileWrapper>> open(
        const std::filesystem::path& path,
        bool create = false,
        std::function<void(Size, Size)> on_read_progress = nullptr) {
        auto fw = std::unique_ptr<FileWrapper>(new FileWrapper());
        auto error = fw->file_.open(path, /*read_only*/ false, create);

        if (error)
            return *error;

        if (on_read_progress)
            fw->on_read_progress = on_read_progress;

        fw->initialize();
        return fw;
    }

    /* Underlying file. */
    File& file() { return file_; }

    /* Swaps out the underlying file for the specified file.
     * The swapped file is expected have the same contents.
     * For copy-on-write scenario with a temp file. */
    bool assume_file(const std::filesystem::path& path) {
        File file;
        auto error = file.open(path, /*read_only*/ false);

        if (error)
            return false;

        file_ = std::move(file);
        return true;
    }

   private:
    FileWrapper() {}
    void initialize() {
        set_buffer(&file_);
    }

    File file_{};
};

template <uint32_t CacheSize = 64, typename T>
BufferWrapper<T, CacheSize> wrap_buffer(T& buffer) {
    return {&buffer};
}

#endif  // __FILE_WRAPPER_HPP__