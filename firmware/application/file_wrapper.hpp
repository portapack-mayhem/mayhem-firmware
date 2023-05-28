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

#include <memory>
#include <string>

enum class LineEnding : uint8_t {
    LF,
    CRLF
};

/* TODO:
 * - CRLF handling.
 */

/* BufferType requires the following members
 * Size size()
 * Result<Size> read(void* data, Size bytes_to_read)
 * Result<Offset> seek(uint32_t offset)
 */

/* Wraps a buffer and provides an API for accessing lines efficiently. */
template <typename BufferType, uint32_t CacheSize>
class BufferWrapper {
   public:
    using Offset = uint32_t;
    using Line = uint32_t;
    using Column = uint32_t;
    using Range = struct {
        // Offset of the line start.
        Offset start;
        // Offset of one past the line end.
        Offset end;
    };

    BufferWrapper(BufferType* buffer)
        : wrapped_{buffer} {
        initialize();
    }
    virtual ~BufferWrapper() {}

    /* Prevent copies */
    BufferWrapper(const BufferWrapper&) = delete;
    BufferWrapper& operator=(const BufferWrapper&) = delete;

    Optional<std::string> get_text(Line line, Column col, Offset length) {
        auto range = line_range(line);
        int32_t to_read = length;

        if (!range)
            return {};

        // Don't read past end of line.
        if (range->start + col + to_read >= range->end)
            to_read = range->end - col - range->start;

        if (to_read <= 0)
            return {};

        return read(range->start + col, to_read);
    }

    /* Gets the size of the buffer in bytes. */
    File::Size size() const { return wrapped_->size(); }

    /* Get the count of the lines in the buffer. */
    uint32_t line_count() const { return line_count_; }

    /* Gets the range of the line if valid. */
    Optional<Range> line_range(Line line) {
        ensure_cached(line);

        auto offset = offset_for_line(line);
        if (!offset)
            return {};

        auto start = *offset == 0 ? start_offset_ : (newlines_[*offset - 1] + 1);
        auto end = newlines_[*offset] + 1;

        return Range{start, end};
    }

    /* Gets the length of the line, or 0 if invalid. */
    Offset line_length(Line line) {
        auto range = line_range(line);

        if (range)
            return range->end - range->start;

        return 0;
    }

    /* Gets the index of the first line in the cache.
     * Only really useful for unit testing or diagnostics. */
    Offset start_line() { return start_line_; };

   protected:
    BufferWrapper() { }

    void set_buffer(BufferType* buffer) {
        wrapped_ = buffer;
        initialize();
    }

   private:
    /* Number of newline offsets to cache. */
    static constexpr Offset max_newlines = CacheSize;

    /* Size of stack buffer used for reading/writing. */
    static constexpr size_t buffer_size = 512;

    void initialize() {
        start_offset_ = 0;
        start_line_ = 0;
        line_count_ = 0;
        newlines_.clear();

        // Special case for empty files to keep them consistent.
        if (size() == 0) {
            line_count_ = 1;
            newlines_.push_back(0);
            return;
        }

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

    Optional<std::string> read(Offset offset, Offset length) {
        if (offset + length > size())
            return {};

        std::string buffer;
        buffer.resize(length);
        wrapped_->seek(offset);

        auto result = wrapped_->read(&buffer[0], length);
        if (result.is_error())
            // TODO: better error handling.
            return std::string{"[Bad Read]"};

        buffer.resize(*result);
        return buffer;
    }

    /* Returns the offset of the line in the newline cache if valid. */
    Optional<Offset> offset_for_line(Line line) const {
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

    /* Helpers for finding the prev/next newline. */
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
    static Result<std::unique_ptr<FileWrapper>> open(const std::filesystem::path& path) {
        auto fw = std::unique_ptr<FileWrapper>(new FileWrapper());
        auto error = fw->file_.open(path);

        if (error)
            return *error;

        fw->initialize();
        return fw;
    }

   private:
    FileWrapper() { }
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