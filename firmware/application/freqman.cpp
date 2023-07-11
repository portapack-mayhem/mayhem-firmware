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

#include "file.hpp"
#include "freqman.hpp"
#include "tone_key.hpp"
#include "ui_widget.hpp"

#include <algorithm>

namespace fs = std::filesystem;
using namespace tonekey;
using namespace ui;

using option_t = std::pair<std::string, int32_t>;
using options_t = std::vector<option_t>;

extern options_t freqman_modulations;
extern options_t freqman_bandwidths[4];
extern options_t freqman_steps;
extern options_t freqman_steps_short;

extern const option_t* find_by_index(const options_t& options, freqman_index_t index);

// TODO: remove in favor of FreqmanDB
/* Freqman file handling. */
bool load_freqman_file(const std::string& file_stem, freqman_db& db, freqman_load_options options) {
    return parse_freqman_file(get_freqman_path(file_stem), db, options);
}

bool delete_freqman_file(const std::string& file_stem) {
    delete_file(get_freqman_path(file_stem));
    return true;
}

bool save_freqman_file(const std::string& file_stem, freqman_db& db) {
    auto path = get_freqman_path(file_stem);
    delete_file(path);

    File freqman_file;
    auto error = freqman_file.create(path);
    if (error)
        return false;

    for (size_t n = 0; n < db.size(); n++)
        freqman_file.write_line(to_freqman_string(*db[n]));

    return true;
}

bool create_freqman_file(const std::string& file_stem) {
    auto fs_error = make_new_file(get_freqman_path(file_stem));
    return fs_error.ok();
}

/* Set options. */
void freqman_set_modulation_option(OptionsField& option) {
    option.set_options(freqman_modulations);
}

void freqman_set_bandwidth_option(freqman_index_t modulation, OptionsField& option) {
    if (is_valid(modulation))
        option.set_options(freqman_bandwidths[modulation]);
}

void freqman_set_step_option(OptionsField& option) {
    option.set_options(freqman_steps);
}

void freqman_set_step_option_short(OptionsField& option) {
    option.set_options(freqman_steps_short);
}

/* Option value lookup. */
// TODO: use Optional instead of magic values.
int32_t freqman_entry_get_modulation_value(freqman_index_t modulation) {
    if (auto opt = find_by_index(freqman_modulations, modulation))
        return opt->second;
    return -1;
}

int32_t freqman_entry_get_bandwidth_value(freqman_index_t modulation, freqman_index_t bandwidth) {
    if (modulation < freqman_modulations.size()) {
        if (auto opt = find_by_index(freqman_bandwidths[modulation], bandwidth))
            return opt->second;
    }
    return -1;
}

int32_t freqman_entry_get_step_value(freqman_index_t step) {
    if (auto opt = find_by_index(freqman_steps, step))
        return opt->second;
    return -1;
}
