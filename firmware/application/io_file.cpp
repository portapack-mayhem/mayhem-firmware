/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
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

#include "string.h"
#include "io_file.hpp"

namespace fs = std::filesystem;
static const fs::path c8_ext = u".C8";

// convert c16 buffer to c8 buffer
// (output buffer size is bytes/2)
void FileWriter::c16_to_c8(const void* buffer, File::Size bytes) {
    int16_t* src = (int16_t*)buffer;
    uint16_t* dest = (uint16_t*)buffer;

    for (File::Size i = 0; i < bytes / 4; i++) {
        uint8_t re = *src++ / 256;
        uint8_t im = *src++ / 256;
        *dest++ = ((uint16_t)im << 8) + re;
    }
}

// convert c8 buffer to c16 buffer
// (output buffer size is 2*bytes)
void FileReader::c8_to_c16(const void* buffer, File::Size bytes) {
    int8_t* src = (int8_t*)buffer + bytes - 1;
    int32_t* dest = (int32_t*)((uint8_t*)buffer + 2 * bytes - 4);

    for (File::Size i = 0; i < bytes / 2; i++) {
        int16_t im = *src-- * 256;
        int16_t re = *src-- * 256;
        *dest-- = ((int32_t)im << 16) + re;
    }
}

Optional<File::Error> FileReader::open(const std::filesystem::path& filename) {
    convert_c8 = path_iequal(filename.extension(), c8_ext);  // automatically enable C8/C16 conversion based on file name
    return file_.open(filename);
}

Optional<File::Error> FileReader::open(const std::filesystem::path& filename, bool do_c8_conversion) {
    convert_c8 = do_c8_conversion;
    return file_.open(filename);
}

// if C8 conversion enabled, half the number of bytes are read from the file & expanded to fill the whole buffer
File::Result<File::Size> FileReader::read(void* const buffer, const File::Size bytes) {
    auto read_result = file_.read(buffer, convert_c8 ? bytes / 2 : bytes);
    if (read_result.is_ok()) {
        bytes_read_ += read_result.value();

        if (convert_c8) {
            c8_to_c16(buffer, read_result.value());
            bytes_read_ += read_result.value();
        }
    }
    return read_result;
}

Optional<File::Error> FileWriter::create(const std::filesystem::path& filename) {
    convert_c8 = path_iequal(filename.extension(), c8_ext);
    return file_.create(filename);
}

// if C8 conversion is enabled, half the number of bytes are written to the file
File::Result<File::Size> FileWriter::write(const void* const buffer, const File::Size bytes) {
    if (convert_c8) {
        c16_to_c8(buffer, bytes);
    }

    auto write_result = file_.write(buffer, convert_c8 ? bytes / 2 : bytes);
    if (write_result.is_ok()) {
        bytes_written_ += convert_c8 ? write_result.value() * 2 : write_result.value();
    }
    return write_result;
}
