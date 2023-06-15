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

#pragma once

#include "file.hpp"
#include <cstring>
#include <cstdio>
#include <string>

/* Mocks the File interface with a backing string. */
class MockFile {
   public:
    using Error = File::Error;
    using Offset = File::Offset;
    using Size = File::Size;
    template <typename T>
    using Result = File::Result<T>;

    MockFile(std::string data)
        : data_{std::move(data)} {}

    Size size() { return data_.size(); }

    Result<Offset> seek(uint32_t offset) {
        if ((int32_t)offset < 0)
            return {static_cast<Error>(FR_BAD_SEEK)};

        auto previous = offset_;

        if (offset > size())
            data_.resize(offset);

        offset_ = offset;
        return previous;
    }

    Result<Offset> truncate() {
        data_.resize(offset_);
        return offset_;
    }

    Result<Size> read(void* data, Size bytes_to_read) {
        if (offset_ + bytes_to_read > size())
            bytes_to_read = size() - offset_;

        if (bytes_to_read == 0 || bytes_to_read > size())  // NB: underflow wrap
            return 0;

        memcpy(data, &data_[offset_], bytes_to_read);
        offset_ += bytes_to_read;
        return bytes_to_read;
    }

    Result<Size> write(const void* data, Size bytes_to_write) {
        auto new_offset = offset_ + bytes_to_write;
        if (new_offset >= size())
            data_.resize(new_offset);

        memcpy(&data_[offset_], data, bytes_to_write);
        offset_ = new_offset;
        return bytes_to_write;
    }

    Optional<Error> sync() {
        return {};
    }

    std::string data_;
    uint32_t offset_{0};
};
