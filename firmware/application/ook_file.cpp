/*
 * Copyright (C) 2024 gullradriel
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

using namespace portapack;
namespace fs = std::filesystem;

/*
    struct of an OOK file:

    Frequency SampleRate SymbolRate Repeat PauseSymbolDuration Payload

    -Frequency is in hertz
    -SampleRate is one of 250k, 1M, 2M, 5M , 10M ,20M
    -SymbolRate is the number of symbols /s
    -Repeat is the number of times we will repeat the payload
    -PauseSymbolDuration is the duration of the pause between repeat, in usec
    -Payload is the payload in form of a string of 0 and 1
*/

bool read_ook_file(const std::filesystem::path& path, ook_file_data& ook_data) {
    File data_file;
    auto open_result = data_file.open(path);
    if (open_result) {
        return false;
    }

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
        std::string symbol_rate_str = line.substr(second_space + 1, third_space - second_space - 1);
        std::string repeat_str = line.substr(third_space + 1, fourth_space - third_space - 1);
        std::string pause_symbol_duration_str = line.substr(fourth_space + 1, fifth_space - fourth_space - 1);
        std::string payload_data = line.substr(fifth_space + 1);  // Extract binary payload as final value

        // Convert and assign frequency
        ook_data.frequency = std::stoull(frequency_str);
        // Convert and assign symbol_rate
        ook_data.symbol_rate = static_cast<unsigned int>(atoi(symbol_rate_str.c_str()));
        // Convert and assign repeat count
        ook_data.repeat = static_cast<unsigned int>(atoi(repeat_str.c_str()));
        // Convert and assign pause_symbol_duration
        ook_data.pause_symbol_duration = static_cast<unsigned int>(atoi(pause_symbol_duration_str.c_str()));
        // Select sample rate based on value read from file
        if (sample_rate_str == "250k") {
            ook_data.sample_rate = 250000U;
        } else if (sample_rate_str == "1M") {
            ook_data.sample_rate = 1000000U;
        } else if (sample_rate_str == "2M") {
            ook_data.sample_rate = 2000000U;
        } else if (sample_rate_str == "5M") {
            ook_data.sample_rate = 5000000U;
        } else if (sample_rate_str == "10M") {
            ook_data.sample_rate = 10000000U;
        } else if (sample_rate_str == "20M") {
            ook_data.sample_rate = 20000000U;
        } else {
            return false;
        }
        // Update payload with binary data
        ook_data.payload = std::move(payload_data);
        // Process only the first line
        break;
    }
    return true;
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
    if (ook_data.sample_rate == 250000U) {
        sample_rate_str = "250k";
    } else if (ook_data.sample_rate == 1000000U) {
        sample_rate_str = "1M";
    } else if (ook_data.sample_rate == 2000000U) {
        sample_rate_str = "2M";
    } else if (ook_data.sample_rate == 5000000U) {
        sample_rate_str = "5M";
    } else if (ook_data.sample_rate == 10000000U) {
        sample_rate_str = "10M";
    } else if (ook_data.sample_rate == 20000000U) {
        sample_rate_str = "20M";
    } else {
        return false;
    }

    // write informations
    src->write_line(to_string_dec_uint(ook_data.frequency) + " " +
                    sample_rate_str + " " +
                    to_string_dec_uint(ook_data.symbol_rate) + " " +
                    to_string_dec_uint(ook_data.repeat) + " " +
                    to_string_dec_uint(ook_data.pause_symbol_duration) + " " +
                    ook_data.payload);

    // Close files
    src.reset();

    return true;
}

void start_ook_file_tx(ook_file_data& ook_data) {
    size_t bitstream_length = encoders::make_bitstream(const_cast<std::string&>(ook_data.payload));  // Convert the message into a bitstream

    transmitter_model.set_target_frequency(ook_data.frequency);  // Set target frequency
    transmitter_model.set_sampling_rate(ook_data.sample_rate);   // Set the OOK sampling rate
    transmitter_model.set_baseband_bandwidth(1750000);           // Set the baseband bandwidth
    transmitter_model.enable();                                  // Enable the transmitter

    // Configure OOK data and transmission characteristics
    baseband::set_ook_data(
        bitstream_length,                             // Length of the bitstream to transmit
        ook_data.sample_rate / ook_data.symbol_rate,  // Calculate symbol period based on repetition rate
        ook_data.repeat,                              // Set the number of times the whole bitstream is repeated
        ook_data.pause_symbol_duration                // Set the pause_symbol between reps
    );
}

void stop_ook_file_tx() {
    transmitter_model.disable();  // Disable the transmitter
}
