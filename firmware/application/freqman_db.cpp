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
// TODO: parse_int seems to hang on invalid input.
bool parse_freqman_entry(std::string_view str, freqman_entry& entry) {
    if (str.empty() || str[0] == '#')
        return false;

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
            // NB: Required modulation to be set.
            if (entry.modulation < std::size(freqman_bandwidths)) {
                entry.step = find_by_name(freqman_bandwidths[entry.modulation], value);
            }
        } else if (key == "c") {
            // Tone
            // // ctcss tone if any
            // pos = strstr(line_start, "c=");
            // if (pos) {
            //     pos += 2;
            //     // find decimal point and replace with 0 if there is one, for strtoll
            //     length = strcspn(pos, ".,\x0A");
            //     if (pos + length <= line_end) {
            //         c = *(pos + length);
            //         *(pos + length) = 0;
            //         // ASCII Hz to integer Hz x 100
            //         tone_freq = strtoll(pos, nullptr, ) * 100;
            //         // stuff saved character back into string in case it was not a decimal point
            //         *(pos + length) = c;
            //         // now get first digit after decimal point (10ths of Hz)
            //         pos += length + 1;
            //         if (c == '.' && *pos >= '0' && *pos <= '9')
            //             tone_freq += (*pos - '0') * 10;
            //         // convert tone_freq (100x the freq in Hz) to a tone_key index
            //         tone = tone_key_index_by_value(tone_freq);
            //     }
            // }
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
        }
    }

    // TODO: Validate?
    return true;
}

bool parse_freqman_file(const fs::path& path, freqman_db& db, freqman_load_options options) {
    File f;
    auto error = f.open(path);
    if (error)
        return false;

    auto reader = FileLineReader(f);
    auto line_count = count_lines(reader);

    // Attempt to avoid a re-alloc if possible.
    db.clear();
    db.reserve(line_count);

    for (const auto& line : reader) {
        freqman_entry entry{};
        if (!parse_freqman_entry(line, entry))
            continue;

        // Filter by entry type.
        if ((entry.type == freqman_type::Single && !options.load_freqs) ||
            (entry.type == freqman_type::Range && !options.load_ranges) ||
            (entry.type == freqman_type::HamRadio && !options.load_hamradios)) {
            continue;
        }

        // Use previous entry's mod/band if current's aren't set.
        if (!db.empty()) {
            if (is_invalid(entry.modulation))
                entry.modulation = db.back()->modulation;
            if (is_invalid(entry.bandwidth))
                entry.bandwidth = db.back()->bandwidth;
        }

        // Move the entry onto the heap and push.
        db.push_back(std::make_unique<freqman_entry>(std::move(entry)));

        // Limit to max_entries when specified.
        if (options.max_entries > 0 && db.size() >= options.max_entries)
            break;
    }

    db.shrink_to_fit();
    return true;
}