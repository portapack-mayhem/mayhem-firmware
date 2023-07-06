/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2023 gullradriel, Nilorea Studio Inc.
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

#include "file.hpp"
#include "freqman.hpp"
#include "tone_key.hpp"

#include <algorithm>

namespace fs = std::filesystem;
using namespace tonekey;

using option_t = std::pair<std::string, int32_t>;
using options_t = std::vector<option_t>;

extern options_t freqman_modulations;
extern options_t freqman_bandwidths[4];
extern options_t freqman_steps;
extern options_t freqman_steps_short;

bool load_freqman_file(const std::string& file_stem, freqman_db& db, freqman_load_options options) {
    fs::path path{u"FREQMAN/"};
    path += file_stem + ".TXT";
    return parse_freqman_file(path, db, options);
}

bool get_freq_string(freqman_entry& entry, std::string& item_string) {
    rf::Frequency frequency_a, frequency_b;

    frequency_a = entry.frequency_a;
    if (entry.type == freqman_type::Single) {
        // Single
        item_string = "f=" + to_string_dec_uint(frequency_a / 1000) + to_string_dec_uint(frequency_a % 1000UL, 3, '0');
    } else if (entry.type == freqman_type::Range) {
        // Range
        frequency_b = entry.frequency_b;
        item_string = "a=" + to_string_dec_uint(frequency_a / 1000) + to_string_dec_uint(frequency_a % 1000UL, 3, '0');
        item_string += ",b=" + to_string_dec_uint(frequency_b / 1000) + to_string_dec_uint(frequency_b % 1000UL, 3, '0');
        if (is_valid(entry.step)) {
            item_string += ",s=" + freqman_entry_get_step_string_short(entry.step);
        }
    } else if (entry.type == freqman_type::HamRadio) {
        frequency_b = entry.frequency_b;
        item_string = "r=" + to_string_dec_uint(frequency_a / 1000) + to_string_dec_uint(frequency_a % 1000UL, 3, '0');
        item_string += ",t=" + to_string_dec_uint(frequency_b / 1000) + to_string_dec_uint(frequency_b % 1000UL, 3, '0');
        if (is_valid(entry.tone)) {
            item_string += ",c=" + tone_key_value_string(entry.tone);
        }
    }
    if (is_valid(entry.modulation) && entry.modulation < freqman_modulations.size()) {
        item_string += ",m=" + freqman_entry_get_modulation_string(entry.modulation);
        if (is_valid(entry.bandwidth) && (unsigned)entry.bandwidth < freqman_bandwidths[entry.modulation].size()) {
            item_string += ",bw=" + freqman_entry_get_bandwidth_string(entry.modulation, entry.bandwidth);
        }
    }
    if (entry.description.size())
        item_string += ",d=" + entry.description;

    return true;
}

bool delete_freqman_file(const std::string& file_stem) {
    File freqman_file;
    std::string freq_file_path = "/FREQMAN/" + file_stem + ".TXT";
    delete_file(freq_file_path);
    return false;
}

bool save_freqman_file(const std::string& file_stem, freqman_db& db) {
    File freqman_file;
    std::string freq_file_path = "/FREQMAN/" + file_stem + ".TXT";
    delete_file(freq_file_path);
    auto result = freqman_file.create(freq_file_path);
    if (!result.is_valid()) {
        for (size_t n = 0; n < db.size(); n++) {
            std::string line;
            get_freq_string(*db[n], line);
            freqman_file.write_line(line);
        }
        return true;
    }
    return false;
}

bool create_freqman_file(const std::string& file_stem, File& freqman_file) {
    auto result = freqman_file.create("FREQMAN/" + file_stem + ".TXT");

    if (result.is_valid())
        return false;

    return true;
}

std::string freqman_item_string(freqman_entry& entry, size_t max_length) {
    std::string item_string;

    switch (entry.type) {
        case freqman_type::Single:
            item_string = to_string_short_freq(entry.frequency_a) + "M: " + entry.description;
            break;
        case freqman_type::Range:
            item_string = "R: " + entry.description;
            break;
        case freqman_type::HamRadio:
            item_string = "H: " + entry.description;
            break;
        default:
            item_string = "!UNKNOWN TYPE " + entry.description;
            break;
    }

    if (item_string.size() > max_length)
        return item_string.substr(0, max_length - 3) + "...";

    return item_string;
}

void freqman_set_modulation_option(OptionsField& option) {
    option.set_options(freqman_modulations);
}

void freqman_set_bandwidth_option(freqman_index_t modulation, OptionsField& option) {
    option.set_options(freqman_bandwidths[modulation]);
}

void freqman_set_step_option(OptionsField& option) {
    option.set_options(freqman_steps);
}

void freqman_set_step_option_short(OptionsField& option) {
    option.set_options(freqman_steps_short);
}

std::string freqman_entry_get_modulation_string(freqman_index_t modulation) {
    if (modulation >= freqman_modulations.size()) {
        return std::string("");  // unknown modulation
    }
    return freqman_modulations[modulation].first;
}

std::string freqman_entry_get_bandwidth_string(freqman_index_t modulation, freqman_index_t bandwidth) {
    if (modulation >= freqman_modulations.size()) {
        return std::string("");  // unknown modulation
    }
    if (bandwidth > freqman_bandwidths[modulation].size()) {
        return std::string("");  // unknown bandwidth for modulation
    }
    return freqman_bandwidths[modulation][bandwidth].first;
}

std::string freqman_entry_get_step_string(freqman_index_t step) {
    if (step >= freqman_steps.size()) {
        return std::string("");  // unknown modulation
    }
    return freqman_steps[step].first;
}

std::string freqman_entry_get_step_string_short(freqman_index_t step) {
    if (step >= freqman_steps_short.size()) {
        return std::string("");  // unknown modulation
    }
    return freqman_steps_short[step].first;
}

int32_t freqman_entry_get_modulation_value(freqman_index_t modulation) {
    if (modulation >= freqman_modulations.size()) {
        return -1;  // unknown modulation
    }
    return freqman_modulations[modulation].second;
}

int32_t freqman_entry_get_bandwidth_value(freqman_index_t modulation, freqman_index_t bandwidth) {
    if (modulation >= freqman_modulations.size()) {
        return -1;  // unknown modulation
    }
    if (bandwidth > freqman_bandwidths[modulation].size()) {
        return -1;  // unknown bandwidth for modulation
    }
    return freqman_bandwidths[modulation][bandwidth].second;
}

int32_t freqman_entry_get_step_value(freqman_index_t step) {
    if (step >= freqman_steps.size()) {
        return -1;  // unknown step
    }
    return freqman_steps[step].second;
}
