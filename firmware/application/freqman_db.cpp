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
#include "file_reader.hpp"
#include "freqman_db.hpp"
#include "string_format.hpp"

using option_t = std::pair<std::string_view, int32_t>;
using options_t = std::vector<option_t>;

options_t freqman_entry_modulations = {
    {"AM", 0},
    {"NFM", 1},
    {"WFM", 2},
    {"SPEC", 3},
};

template <typename T>
T parse(std::string_view str, 

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
            parse_int(value, entry.frequency_a);
            entry.type = freqman_type::Range;
        } else if (key == "b") {
            parse_int(value, entry.frequency_b);
        } else if (key == "bw") {

        } else if (key == "d") {
            entry.description = trim(value);
        } else if (key == "f") {
            parse_int(value, entry.frequency_a);
            entry.type = freqman_type::Single;
        } else if (key == "m") {
            entry.modulation = parse_modulation(value);
        } else if (key == "r") {
            parse_int(value, entry.frequency_a);
            entry.type = freqman_type::HamRadio;
        } else if (key == "s") {
            entry.step = parse_step(value);
        } else if (key == "t") {
            parse_int(value, entry.frequency_b);
        } else {
            // Invalid.
        }
    }

    // TODO: Validate?
    return true;
}
