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

#include "metadata_file.hpp"

#include "convert.hpp"
#include "file_reader.hpp"
#include "string_format.hpp"
#include <string_view>

namespace fs = std::filesystem;
using namespace std::literals;

const std::string_view center_freq_name = "center_frequency"sv;
const std::string_view sample_rate_name = "sample_rate"sv;
const std::string_view latitude_name = "latitude"sv;
const std::string_view longitude_name = "longitude"sv;
const std::string_view satinuse_name = "satinuse"sv;

fs::path get_metadata_path(const fs::path& capture_path) {
    auto temp = capture_path;
    return temp.replace_extension(u".TXT");
}

Optional<File::Error> write_metadata_file(const fs::path& path, capture_metadata metadata) {
    File f;
    auto error = f.create(path);

    if (error)
        return error;

    error = f.write_line(std::string{center_freq_name} + "=" +
                         to_string_dec_uint(metadata.center_frequency));
    if (error)
        return error;

    error = f.write_line(std::string{sample_rate_name} + "=" +
                         to_string_dec_uint(metadata.sample_rate));
    if (error)
        return error;

    // add gps data if available
    if (metadata.latitude != 0 && metadata.longitude != 0 && metadata.latitude < 200 && metadata.longitude < 200) {
        error = f.write_line(std::string{latitude_name} + "=" +
                             to_string_decimal(metadata.latitude, 7));
        if (error)
            return error;

        error = f.write_line(std::string{longitude_name} + "=" +
                             to_string_decimal(metadata.longitude, 7));
        if (error)
            return error;

        error = f.write_line(std::string{satinuse_name} + "=" +
                             to_string_dec_uint(metadata.satinuse));
        if (error)
            return error;
    }
    return {};
}

Optional<capture_metadata> read_metadata_file(const fs::path& path) {
    File f;
    auto error = f.open(path);

    if (error)
        return {};

    capture_metadata metadata{};

    auto reader = FileLineReader(f);
    for (const auto& line : reader) {
        auto cols = split_string(line, '=');

        if (cols.size() != 2)
            continue;  // Bad line.

        if (cols[0] == center_freq_name)
            parse_int(cols[1], metadata.center_frequency);
        else if (cols[0] == sample_rate_name)
            parse_int(cols[1], metadata.sample_rate);
        else if (cols[0] == latitude_name)
            parse_float_meta(cols[1], metadata.latitude);
        else if (cols[0] == longitude_name)
            parse_float_meta(cols[1], metadata.longitude);
        else if (cols[0] == satinuse_name)
            parse_int(cols[1], metadata.satinuse);
        else
            continue;
    }

    if (metadata.center_frequency == 0 || metadata.sample_rate == 0)
        return {};  // Parse failed.

    return metadata;
}

bool parse_float_meta(std::string_view str, float& out_val) {
    out_val = {};

    if (str.size() > max_parse_int_length)
        return false;

    // Copy onto the stack and null terminate.
    char zstr[max_parse_int_length + 1];
    std::memcpy(zstr, str.data(), str.size());
    zstr[str.size()] = '\0';
    errno = 0;
    out_val = strtod(zstr, nullptr);
    return (errno == 0);
}