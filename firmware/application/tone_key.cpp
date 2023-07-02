/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#include "string_format.hpp"
#include "tone_key.hpp"

namespace tonekey {

// Keep list in ascending order by tone frequency
const tone_key_t tone_keys = {
    {"None", 0.0},
    {"1 XZ", 67.000},
    {"39 WZ", 69.300},
    {"2 XA", 71.900},
    {"3 WA", 74.400},
    {"4 XB", 77.000},
    {"5 WB", 79.700},
    {"6 YZ", 82.500},
    {"7 YA", 85.400},
    {"8 YB", 88.500},
    {"9 ZZ", 91.500},
    {"10 ZA", 94.800},
    {"11 ZB", 97.400},
    {"12 1Z", 100.000},
    {"13 1A", 103.500},
    {"14 1B", 107.200},
    {"15 2Z", 110.900},
    {"16 2A", 114.800},
    {"17 2B", 118.800},
    {"18 3Z", 123.000},
    {"19 3A", 127.300},
    {"20 3B", 131.800},
    {"21 4Z", 136.500},
    {"22 4A", 141.300},
    {"23 4B", 146.200},
    {"24 5Z", 151.400},
    {"25 5A", 156.700},
    {"40 --", 159.800},
    {"26 5B", 162.200},
    {"41 --", 165.500},
    {"27 6Z", 167.900},
    {"42 --", 171.300},
    {"28 6A", 173.800},
    {"43 --", 177.300},
    {"29 6B", 179.900},
    {"44 --", 183.500},
    {"30 7Z", 186.200},
    {"45 --", 189.900},
    {"31 7A", 192.800},
    {"46 --", 196.600},
    {"47 --", 199.500},
    {"32 M1", 203.500},
    {"48 8Z", 206.500},
    {"33 M2", 210.700},
    {"34 M3", 218.100},
    {"35 M4", 225.700},
    {"49 9Z", 229.100},
    {"36 M5", 233.600},
    {"37 M6", 241.800},
    {"38 M7", 250.300},
    {"50 0Z", 254.100},
    {"Shure 19kHz", 19000.0},
    {"Axient 28kHz", 28000.0},
    {"Senn. 32.000k", 32000.0},
    {"Sony 32.382k", 32382.0},
    {"Senn. 32.768k", 32768.0}};

void tone_keys_populate(OptionsField& field) {
    using option_t = std::pair<std::string, int32_t>;
    using options_t = std::vector<option_t>;
    options_t tone_key_options;
    std::string tone_name;

    for (size_t c = 1; c < tone_keys.size(); c++) {
        auto f = tone_keys[c].second;
        if (f < 1000.0) {
            tone_name = "CTCSS " + to_string_dec_uint(f) + "." + to_string_dec_uint((uint32_t)(f * 10) % 10) + " " + tone_keys[c].first;
        } else {
            tone_name = tone_keys[c].first;
        }

        tone_key_options.emplace_back(tone_name, c);
    }

    field.set_options(tone_key_options);
}

float tone_key_frequency(const tone_index index) {
    return tone_keys[index].second;
}

std::string tone_key_string(tone_index index) {
    if (index < 0 || (unsigned)index >= tone_keys.size())
        return std::string("");
    return tone_keys[index].first;
}

// Return string showing frequency only from specific table index
std::string tone_key_value_string(tone_index index) {
    if (index < 0 || (unsigned)index >= tone_keys.size())
        return std::string("");
    auto f = tone_keys[index].second;
    return to_string_dec_uint((uint32_t)f) + "." + to_string_dec_uint((uint32_t)(f * 10) % 10);
}

// Return variable-length string showing CTCSS tone from tone frequency
// Value is in 0.01 Hz units
std::string tone_key_string_by_value(uint32_t value, size_t max_length) {
    static uint8_t tone_display_toggle{0};
    static uint32_t last_value;
    tone_index idx;
    std::string freq_str;

    // If >10Hz difference between consecutive samples, it's probably noise, so ignore
    if (abs(value - last_value) > 10 * 100) {
        last_value = value;
        tone_display_toggle = 0;
        return "        ";
    }
    last_value = value;

    // Only display 1/10 Hz accuracy if <1000 Hz; max 5 characters
    if (value >= 1000 * 100)
        freq_str = to_string_dec_int(value / 100);
    else
        freq_str = to_string_dec_int(value / 100) + "." + to_string_dec_uint((value / 10) % 10);

    if (max_length >= 3 + 5 + 1 + 5) {
        return "T: " + freq_str + " " + tone_key_string(tone_key_index_by_value(value));
    } else {
        // Not enough space; toggle between display of tone received and tone code #
        if (tone_display_toggle++ < TONE_DISPLAY_TOGGLE_COUNTER) {
            return "T: " + freq_str;
        } else {
            if (tone_display_toggle >= TONE_DISPLAY_TOGGLE_COUNTER * 2)
                tone_display_toggle = 0;

            // If we don't find a close tone freq in the table, display frequency instead
            idx = tone_key_index_by_value(value);
            if (idx == -1)
                return "T: " + freq_str;
            else
                return "T: " + tone_key_string(idx);
        }
    }
}

// Search tone_key table for tone frequency value
// Value is in 0.01 Hz units
tone_index tone_key_index_by_value(uint32_t value) {
    float diff;
    float min_diff{(float)value};
    float fvalue{(float)(min_diff / 100.0)};
    tone_index min_idx{-1};
    tone_index idx;

    // Find nearest match
    for (idx = 0; idx < (tone_index)tone_keys.size(); idx++) {
        diff = abs(fvalue - tone_keys[idx].second);
        if (diff < min_diff) {
            min_idx = idx;
            min_diff = diff;
        } else {
            // list is sorted in frequency order; if diff is getting larger than we've passed it
            break;
        }
    }

    // Arbitrary confidence threshold in Hz
    if (min_diff < TONE_FREQ_TOLERANCE_HZ)
        return min_idx;
    else
        return -1;
}

tone_index tone_key_index_by_string(char* str) {
    if (!str)
        return -1;
    for (tone_index index = 0; (unsigned)index < tone_keys.size(); index++) {
        if (tone_keys[index].first.compare(str) >= 0)
            return index;
    }
    return -1;
}

}  // namespace tonekey
