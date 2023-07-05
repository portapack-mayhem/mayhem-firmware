/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2023 gullradriel, Nilorea Studio Inc.
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

#include "convert.hpp"
#include "file.hpp"
#include "file_reader.hpp"
#include "freqman_db.hpp"
#include "string_format.hpp"

#include <array>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

using option_t = std::pair<std::string, int32_t>;
using options_t = std::vector<option_t>;

options_t freqman_modulations = {
    {"AM", 0},
    {"NFM", 1},
    {"WFM", 2},
    {"SPEC", 3},
};

options_t freqman_bandwidths[4] = {
    {
        // AM
        {"DSB 9k", 0},
        {"DSB 6k", 1},
        {"USB+3k", 2},
        {"LSB-3k", 3},
        {"CW", 4},
    },
    {
        // NFM
        {"8k5", 0},
        {"11k", 1},
        {"16k", 2},
    },
    {
        // WFM
        {"40k", 2},
        {"180k", 1},
        {"200k", 0},
    },
    {
        // SPEC -- TODO: these should be indexes.
        {"8k5", 8500},
        {"11k", 11000},
        {"16k", 16000},
        {"25k", 25000},
        {"50k", 50000},
        {"100k", 100000},
        {"250k", 250000},
        {"500k", 500000}, /* Previous Limit bandwith Option with perfect micro SD write .C16 format operaton.*/
        {"600k", 600000}, /* That extended option is still possible to record with FW version Mayhem v1.41 (< 2,5MB/sec) */
        {"650k", 650000},
        {"750k", 750000}, /* From this BW onwards, the LCD is ok, but the recorded file is decimated, (not real file size) */
        {"1100k", 1100000},
        {"1750k", 1750000},
        {"2000k", 2000000},
        {"2500k", 2500000},
        {"2750k", 2750000},  // That is our max Capture option, to keep using later / 8 decimation (22Mhz sampling  ADC)
    },
};

options_t freqman_steps = {
    {"0.1kHz      ", 100},
    {"1kHz        ", 1000},
    {"5kHz (SA AM)", 5000},
    {"6.25kHz(NFM)", 6250},
    {"8.33kHz(AIR)", 8330},
    {"9kHz (EU AM)", 9000},
    {"10kHz(US AM)", 10000},
    {"12.5kHz(NFM)", 12500},
    {"15kHz  (HFM)", 15000},
    {"25kHz   (N1)", 25000},
    {"30kHz (OIRT)", 30000},
    {"50kHz  (FM1)", 50000},
    {"100kHz (FM2)", 100000},
    {"250kHz  (N2)", 250000},
    {"500kHz (WFM)", 500000},
    {"1MHz        ", 1000000},
};

options_t freqman_steps_short = {
    {"0.1kHz", 100},
    {"1kHz", 1000},
    {"5kHz", 5000},
    {"6.25kHz", 6250},
    {"8.33kHz", 8330},
    {"9kHz", 9000},
    {"10kHz", 10000},
    {"12.5kHz", 12500},
    {"15kHz", 15000},
    {"25kHz", 25000},
    {"30kHz", 30000},
    {"50kHz", 50000},
    {"100kHz", 100000},
    {"250kHz", 250000},
    {"500kHz", 500000},
    {"1MHz", 1000000},
};

uint8_t find_by_name(const options_t& options, std::string_view name) {
    for (auto ix = 0u; ix < options.size(); ++ix)
        if (options[ix].first == name)
            return ix;

    return freqman_invalid_index;
}

template <typename T, size_t N>
const T* find_by_name(const std::array<T, N>& info, std::string_view name) {
    for (const auto& it : info) {
        if (it.name == name)
            return &it;
    }

    return nullptr;
}

// TODO: How much format validation should this do?
// It's very permissive right now, but entries can be invalid.
bool parse_freqman_entry(std::string_view str, freqman_entry& entry) {
    auto cols = split_string(str, ',');
    entry = freqman_entry{};

    for (auto col : cols) {
        if (col.empty())
            continue;

        auto pair = split_string(col, '=');
        if (pair.size() != 2)
            continue;

        auto key = pair[0];
        auto value = pair[1];

        if (key == "a") {
            entry.type = freqman_type::Range;
            parse_int(value, entry.frequency_a);
        } else if (key == "b") {
            parse_int(value, entry.frequency_b);
        } else if (key == "bw") {
            // Bandwidth
        } else if (key == "c") {
            // Tone
        } else if (key == "d") {
            entry.description = trim(value);
        } else if (key == "f") {
            entry.type = freqman_type::Single;
            parse_int(value, entry.frequency_a);
        } else if (key == "m") {
            entry.modulation = find_by_name(freqman_modulations, value);
        } else if (key == "r") {
            entry.type = freqman_type::HamRadio;
            parse_int(value, entry.frequency_a);
        } else if (key == "s") {
            entry.step = find_by_name(freqman_steps_short, value);
        } else if (key == "t") {
            parse_int(value, entry.frequency_b);
        } else {
            // Invalid.
        }
    }

    // TODO: Validate?
    return true;
}

freqman_db parse_freqman_file(const fs::path& path) {
    File f;
    freqman_db db;

    auto error = f.open(path);
    if (error)
        return db;

    auto reader = FileLineReader(f);
    auto line_count = count_lines(reader);

    db.reserve(line_count);
    for (const auto& line : reader) {
        freqman_entry_ptr entry{};
        if (!parse_freqman_entry(line, *entry))
            continue;

        db.push_back(std::move(entry));
    }

    return db;
}