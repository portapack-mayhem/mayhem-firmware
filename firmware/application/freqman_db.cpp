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
#include "tone_key.hpp"

#include <array>
#include <cctype>
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

/* Impl for next round of changes.
 *template <typename T, size_t N>
 *const T* find_by_name(const std::array<T, N>& info, std::string_view name) {
 *    for (const auto& it : info) {
 *        if (it.name == name)
 *            return &it;
 *    }
 *
 *    return nullptr;
 *}
 */

bool parse_freqman_entry(std::string_view str, freqman_entry& entry) {
    if (str.empty() || str[0] == '#')
        return false;

    entry = freqman_entry{};
    auto cols = split_string(str, ',');

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
            // NB: Requires modulation to be set first
            if (entry.modulation < std::size(freqman_bandwidths)) {
                entry.bandwidth = find_by_name(freqman_bandwidths[entry.modulation], value);
            }
        } else if (key == "c") {
            // Split into whole and fractional parts.
            auto parts = split_string(value, '.');
            int32_t tone_freq = 0;
            int32_t whole_part = 0;
            parse_int(parts[0], whole_part);

            // Tones are stored as frequency / 100 for some reason.
            // E.g. 14572 would be 145.7 (NB: 1s place is dropped).
            // TODO: Might be easier to just store the codes?
            // Multiply the whole part by 100 to get the tone frequency.
            tone_freq = whole_part * 100;

            // Add the fractional part, if present.
            if (parts.size() > 1) {
                auto c = parts[1].front();
                auto digit = std::isdigit(c) ? c - '0' : 0;
                tone_freq += digit * 10;
            }
            entry.tone = static_cast<freqman_index_t>(
                tonekey::tone_key_index_by_value(tone_freq));
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

    // No valid frequency combination was set.
    if (entry.type == freqman_type::Unknown)
        return false;

    // Ranges should have both frequencies set and A <= B.
    if (entry.type == freqman_type::Range || entry.type == freqman_type::HamRadio) {
        if (entry.frequency_a == 0 || entry.frequency_b == 0)
            return false;

        if (entry.frequency_a > entry.frequency_b)
            return false;
    }

    // TODO: Consider additional validation:
    // - Tone only on HamRadio.
    // - Fail on failed parse_int.
    // - Fail if bandwidth set before modulation.

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