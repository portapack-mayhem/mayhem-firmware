/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#include "string_format.hpp"
#include "tone_key.hpp"

namespace tonekey {

// Keep list in ascending order by tone frequency
const tone_key_t tone_keys = {
    {"None", F2Ix100(0.0)},
    {"1 XZ", F2Ix100(67.0)},
    {"39 WZ", F2Ix100(69.3)},
    {"2 XA", F2Ix100(71.9)},
    {"3 WA", F2Ix100(74.4)},
    {"4 XB", F2Ix100(77.0)},
    {"5 WB", F2Ix100(79.7)},
    {"6 YZ", F2Ix100(82.5)},
    {"7 YA", F2Ix100(85.4)},
    {"8 YB", F2Ix100(88.5)},
    {"9 ZZ", F2Ix100(91.5)},
    {"10 ZA", F2Ix100(94.8)},
    {"11 ZB", F2Ix100(97.4)},
    {"12 1Z", F2Ix100(100.0)},
    {"13 1A", F2Ix100(103.5)},
    {"14 1B", F2Ix100(107.2)},
    {"15 2Z", F2Ix100(110.9)},
    {"16 2A", F2Ix100(114.8)},
    {"17 2B", F2Ix100(118.8)},
    {"18 3Z", F2Ix100(123.0)},
    {"19 3A", F2Ix100(127.3)},
    {"20 3B", F2Ix100(131.8)},
    {"21 4Z", F2Ix100(136.5)},
    {"22 4A", F2Ix100(141.3)},
    {"23 4B", F2Ix100(146.2)},
    {"24 5Z", F2Ix100(151.4)},
    {"25 5A", F2Ix100(156.7)},
    {"40 --", F2Ix100(159.8)},
    {"26 5B", F2Ix100(162.2)},
    {"41 --", F2Ix100(165.5)},
    {"27 6Z", F2Ix100(167.9)},
    {"42 --", F2Ix100(171.3)},
    {"28 6A", F2Ix100(173.8)},
    {"43 --", F2Ix100(177.3)},
    {"29 6B", F2Ix100(179.9)},
    {"44 --", F2Ix100(183.5)},
    {"30 7Z", F2Ix100(186.2)},
    {"45 --", F2Ix100(189.9)},
    {"31 7A", F2Ix100(192.8)},
    {"46 --", F2Ix100(196.6)},
    {"47 --", F2Ix100(199.5)},
    {"32 M1", F2Ix100(203.5)},
    {"48 8Z", F2Ix100(206.5)},
    {"33 M2", F2Ix100(210.7)},
    {"34 M3", F2Ix100(218.1)},
    {"35 M4", F2Ix100(225.7)},
    {"49 9Z", F2Ix100(229.1)},
    {"36 M5", F2Ix100(233.6)},
    {"37 M6", F2Ix100(241.8)},
    {"38 M7", F2Ix100(250.3)},
    {"50 0Z", F2Ix100(254.1)},
    {"Shure 19kHz", F2Ix100(19000.0)},
    {"Axient 28kHz", F2Ix100(28000.0)},
    {"Senn. 32.000k", F2Ix100(32000.0)},
    {"Sony 32.382k", F2Ix100(32382.0)},
    {"Senn. 32.768k", F2Ix100(32768.0)}};

std::string fx100_string(uint32_t f) {
    return to_string_dec_uint((f + 5) / 100) + "." + to_string_dec_uint(((f + 5) / 10) % 10);
}

float tone_key_frequency(tone_index index) {
    return float(tone_keys[index].second) / 100.0;
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
    return fx100_string(tone_keys[index].second);
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
    if (value < 1000 * 100)
        freq_str = "T:" + fx100_string(value);
    else
        freq_str = "T:" + to_string_dec_uint(value / 100);

    // Check field length is enough for character counts in the string below
    if (max_length >= 7 + 2 + 5) {
        idx = tone_key_index_by_value(value);
        if (idx != -1)
            return freq_str + " #" + tone_key_string(idx);
    } else {
        // Not enough space; toggle between display of tone received and tone code #
        if (tone_display_toggle++ >= TONE_DISPLAY_TOGGLE_COUNTER) {
            if (tone_display_toggle >= TONE_DISPLAY_TOGGLE_COUNTER * 2) tone_display_toggle = 0;

            // Look for a match in the table (otherwise just display frequency)
            idx = tone_key_index_by_value(value);
            if (idx != -1)
                return "T:" + tone_key_string(idx);
        }
    }
    return freq_str;
}

// Search tone_key table for tone frequency value
// Value is in 0.01 Hz units
tone_index tone_key_index_by_value(uint32_t value) {
    uint32_t diff;
    uint32_t min_diff{UINT32_MAX};
    tone_index min_idx{-1};
    tone_index idx;

    // Find nearest match
    for (idx = 0; idx < (tone_index)tone_keys.size(); idx++) {
        diff = abs(value - tone_keys[idx].second);
        if (diff < min_diff) {
            min_idx = idx;
            min_diff = diff;
        } else {
            // list is sorted in frequency order; if diff is getting larger than we've passed it
            break;
        }
    }

    // Arbitrary confidence threshold
    if (min_diff < TONE_FREQ_TOLERANCE_CENTIHZ)
        return min_idx;
    else
        return -1;
}

tone_index tone_key_index_by_string(char* str) {
    if (!str)
        return -1;
    for (tone_index index = 0; (unsigned)index < tone_keys.size(); index++) {
        if (tone_keys[index].first.compare(str) >= 0)  // TODO: why >=?
            return index;
    }
    return -1;
}

}  // namespace tonekey
