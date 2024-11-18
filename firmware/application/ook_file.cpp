/*
 * Copyright (C) 2024 HTotoo
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

#include "ook_file.hpp"

#include "convert.hpp"
#include "file_reader.hpp"
#include "string_format.hpp"
#include <string_view>

namespace fs = std::filesystem;

/*
    struct of an OOK file:

    Frequency SampleRate BitDuration Repeat PauseDuration Payload

    -Frequency is in hertz
    -SampleRate is one of 250k, 1M, 2M, 5M , 10M ,20M
    -BitDuration is the duration of bits in usec
    -Repeat is the number of times we will repeat the payload
    -PauseDuration is the duration of the pause between repeat, in usec
    -Payload is the payload in form of a string of 0 and 1
*/

ook_file_data* read_ook_file(const fs::path& path) {
    File data_file;
    auto open_result = data_file.open(path);
    if (open_result) {
        return nullptr;
    }

    ook_file_data* ook_data = new ook_file_data();

    FileLineReader reader(data_file);

    for (const auto& line : reader) {
        // Split the line into segments to extract each value
        size_t first_space = line.find(' ');
        size_t second_space = line.find(' ', first_space + 1);
        size_t third_space = line.find(' ', second_space + 1);
        size_t fourth_space = line.find(' ', third_space + 1);
        size_t fifth_space = line.find(' ', fourth_space + 1);

        // Extract each component of the line
        std::string frequency_str = line.substr(0, first_space);
        std::string sample_rate_str = line.substr(first_space + 1, second_space - first_space - 1);
        std::string bit_duration_str = line.substr(second_space + 1, third_space - second_space - 1);
        std::string repeat_str = line.substr(third_space + 1, fourth_space - third_space - 1);
        std::string pause_duration_str = line.substr(fourth_space + 1, fifth_space - fourth_space - 1);
        std::string payload_data = line.substr(fifth_space + 1);  // Extract binary payload as final value

        // Convert and assign frequency
        ook_data->frequency = std::stoull(frequency_str);
        // Convert and assign bit_duration
        ook_data->bit_duration = static_cast<unsigned int>(atoi(bit_duration_str.c_str()));
        // Convert and assign repeat count
        ook_data->repeat = static_cast<unsigned int>(atoi(repeat_str.c_str()));
        // Convert and assign pause_duration
        ook_data->pause_duration = static_cast<unsigned int>(atoi(pause_duration_str.c_str()));
        // Select sample rate based on value read from file
        if (sample_rate_str == "250k") {
            ook_data->samplerate = 250000U;
        } else if (sample_rate_str == "1M") {
            ook_data->samplerate = 1000000U;
        } else if (sample_rate_str == "2M") {
            ook_data->samplerate = 2000000U;
        } else if (sample_rate_str == "5M") {
            ook_data->samplerate = 5000000U;
        } else if (sample_rate_str == "10M") {
            ook_data->samplerate = 10000000U;
        } else if (sample_rate_str == "20M") {
            ook_data->samplerate = 20000000U;
        } else {
            delete ook_data;
            return nullptr;
        }
        // Update payload with binary data
        ook_data->payload = payload_data;
        // Process only the first line
        break;
    }
    return ook_data;
}

bool save_ook_file(ook_file_data& ook_data, const std::filesystem::path& path) {
    // delete file if it exists
    delete_file(path);

    // Attempt to open, if it can't be opened. Create new.
    auto src = std::make_unique<File>();
    auto error = src->open(path, false, true);
    if (error) {
        return false;
    }

    std::string sample_rate_str;
    if (ook_data.samplerate == 250000U) {
        sample_rate_str = "250k";
    } else if (ook_data.samplerate == 1000000U) {
        sample_rate_str = "1M";
    } else if (ook_data.samplerate == 2000000U) {
        sample_rate_str = "2M";
    } else if (ook_data.samplerate == 5000000U) {
        sample_rate_str = "5M";
    } else if (ook_data.samplerate == 10000000U) {
        sample_rate_str = "10M";
    } else if (ook_data.samplerate == 20000000U) {
        sample_rate_str = "20M";
    } else {
        return false;
    }

    // write informations
    src->write_line(to_string_dec_uint(ook_data.frequency) + " " +
                    sample_rate_str + " " +
                    to_string_dec_uint(ook_data.bit_duration) + " " +
                    to_string_dec_uint(ook_data.repeat) + " " +
                    to_string_dec_uint(ook_data.pause_duration) + " " +
                    ook_data.payload);

    // Close files
    src.reset();

    return true;
}
