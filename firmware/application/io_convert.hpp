/*
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

#pragma once

#include "io_file.hpp"

#include "io.hpp"
#include "file.hpp"
#include "optional.hpp"

#include <cstdint>

namespace file_convert {

void c8_to_c16(const void* buffer, File::Size bytes);
void c16_to_c8(const void* buffer, File::Size bytes);

} /* namespace file_convert */

class FileConvertReader : public stream::Reader {
   public:
    FileConvertReader() = default;

    FileConvertReader(const FileConvertReader&) = delete;
    FileConvertReader& operator=(const FileConvertReader&) = delete;
    FileConvertReader(FileConvertReader&& file) = delete;
    FileConvertReader& operator=(FileConvertReader&&) = delete;

    Optional<File::Error> open(const std::filesystem::path& filename);

    File::Result<File::Size> read(void* const buffer, const File::Size bytes) override;
    const File& file() const& { return file_; }

    bool convert_c8_to_c16{};

   protected:
    File file_{};
    uint64_t bytes_read_{0};
};

class FileConvertWriter : public stream::Writer {
   public:
    FileConvertWriter() = default;

    FileConvertWriter(const FileConvertWriter&) = delete;
    FileConvertWriter& operator=(const FileConvertWriter&) = delete;
    FileConvertWriter(FileConvertWriter&& file) = delete;
    FileConvertWriter& operator=(FileConvertWriter&&) = delete;

    Optional<File::Error> create(const std::filesystem::path& filename);

    File::Result<File::Size> write(const void* const buffer, const File::Size bytes) override;
    const File& file() const& { return file_; }

    bool convert_c16_to_c8{};

   protected:
    File file_{};
    uint64_t bytes_written_{0};
};
