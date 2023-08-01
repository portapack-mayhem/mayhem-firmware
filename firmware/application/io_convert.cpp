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

#include "io_convert.hpp"
#include "complex.hpp"

namespace fs = std::filesystem;
static const fs::path c8_ext = u".C8";

namespace file_convert {

// Convert buffer contents from c16 to c8.
// Same buffer used for input & output; input size is bytes; output size is bytes/2.
void c16_to_c8(const void* buffer, File::Size bytes) {
    complex16_t* src = (complex16_t*)buffer;
    complex8_t* dest = (complex8_t*)buffer;

    // Shift isn't used here because it would amplify noise at center freq since it's a signed number
    // i.e. ((-1 >> 8) << 8) = -256, whereas (-1 / 256) * 256 = 0
    for (File::Size i = 0; i < bytes / sizeof(complex16_t); i++) {
        auto re_out = src[i].real() / 256;
        auto im_out = src[i].imag() / 256;
        dest[i] = {(int8_t)re_out, (int8_t)im_out};
    }
}

// Convert c8 buffer to c16 buffer.
// Same buffer used for input & output; input size is bytes; output size is 2*bytes.
void c8_to_c16(const void* buffer, File::Size bytes) {
    complex8_t* src = (complex8_t*)buffer;
    complex16_t* dest = (complex16_t*)buffer;
    uint32_t i = bytes / sizeof(complex8_t);

    if (i != 0) {
        do {
            i--;
            auto re_out = (int16_t)src[i].real() * 256;  // C8 to C16 conversion;
            auto im_out = (int16_t)src[i].imag() * 256;  // Can't shift signed numbers left, so using multiply
            dest[i] = {(int16_t)re_out, (int16_t)im_out};
        } while (i != 0);
    }
}

} /* namespace file_convert */

// Automatically enables C8/C16 conversion based on file extension
Optional<File::Error> FileConvertReader::open(const std::filesystem::path& filename) {
    convert_c8_to_c16 = path_iequal(filename.extension(), c8_ext);
    return file_.open(filename);
}

// If C8 conversion enabled, half the number of bytes are read from the file & expanded to fill the whole buffer.
File::Result<File::Size> FileConvertReader::read(void* const buffer, const File::Size bytes) {
    auto read_result = file_.read(buffer, convert_c8_to_c16 ? bytes / 2 : bytes);
    if (read_result.is_ok()) {
        if (convert_c8_to_c16) {
            file_convert::c8_to_c16(buffer, read_result.value());
            read_result = read_result.value() * 2;
        }
        bytes_read_ += read_result.value();
    }
    return read_result;
}

// Automatically enables C8/C16 conversion based on file extension
Optional<File::Error> FileConvertWriter::create(const std::filesystem::path& filename) {
    convert_c16_to_c8 = path_iequal(filename.extension(), c8_ext);
    return file_.create(filename);
}

// If C8 conversion is enabled, half the number of bytes are written to the file.
File::Result<File::Size> FileConvertWriter::write(const void* const buffer, const File::Size bytes) {
    if (convert_c16_to_c8) {
        file_convert::c16_to_c8(buffer, bytes);
    }
    auto write_result = file_.write(buffer, convert_c16_to_c8 ? bytes / 2 : bytes);
    if (write_result.is_ok()) {
        if (convert_c16_to_c8) {
            write_result = write_result.value() * 2;
        }
        bytes_written_ += write_result.value();
    }
    return write_result;
}
