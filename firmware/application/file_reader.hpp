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

#ifndef __FILE_READER_HPP__
#define __FILE_READER_HPP__

#include "file.hpp"
#include <cstring>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

/* BufferType requires the following members
 * Size size()
 * Result<Size> read(void* data, Size bytes_to_read)
 * Result<Offset> seek(uint32_t offset)
 */

/* Iterates lines in buffer split on '\n'.
 * NB: very basic iterator impl, don't try anything fancy with it.
 * For example, you _must_ deref the iterator after advancing it. */
template <typename BufferType>
class BufferLineReader {
   public:
    struct iterator {
        bool operator!=(const iterator& other) {
            return this->pos_ != other.pos_ || this->reader_ != other.reader_;
        }

        const std::string& operator*() {
            if (!cached_) {
                bool ok = reader_->read_line(*this);
                cached_ = true;

                if (!ok) *this = reader_->end();
            }

            return line_data_;
        }

        iterator& operator++() {
            const auto size = reader_->size();

            if (pos_ < size) {
                cached_ = false;
                pos_ += line_data_.length();
            }

            if (pos_ >= size)
                *this = reader_->end();

            return *this;
        }

        typename BufferType::Size pos_{};
        BufferLineReader* reader_{};
        bool cached_ = false;
        std::string line_data_{};
    };

    BufferLineReader(BufferType& buffer)
        : buffer_{buffer} {}

    iterator begin() { return {0, this}; }
    iterator end() { return {size(), this}; }

    typename BufferType::Size size() const { return buffer_.size(); }

   private:
    BufferType& buffer_;

    bool read_line(iterator& it) {
        constexpr size_t buf_size = 0x80;
        char buf[buf_size];
        uint32_t offset = 0;

        it.line_data_.resize(buf_size);
        buffer_.seek(it.pos_);

        while (true) {
            auto read = buffer_.read(buf, buf_size);
            if (!read)
                return false;

            // Find newline.
            auto len = 0u;
            for (; len < *read; ++len) {
                if (buf[len] == '\n') {
                    ++len;
                    break;
                }
            }

            // Reallocate if needed.
            if (offset + len >= it.line_data_.length())
                it.line_data_.resize(offset + len);

            std::strncpy(&it.line_data_[offset], buf, len);
            offset += len;

            if (len < buf_size)
                break;
        }

        it.line_data_.resize(offset);
        return true;
    }
};

using FileLineReader = BufferLineReader<File>;

/* Splits the string on the specified char and returns
 * a vector of string_views. NB: the lifetime of the
 * string to split must be maintained while the views
 * are used or they will dangle. */
std::vector<std::string_view> split_string(std::string_view str, char c);

/* Returns the number of lines in a file. */
template <typename BufferType>
uint32_t count_lines(BufferLineReader<BufferType>& reader) {
    uint32_t count = 0;
    auto it = reader.begin();

    do {
        *it;  // Necessary to force the file read.
        ++count;
    } while (++it != reader.end());

    return count;
}

#endif
