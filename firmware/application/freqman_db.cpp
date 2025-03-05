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
#include "utility.hpp"
#include "file_path.hpp"

#include <array>
#include <cctype>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

const std::filesystem::path freqman_extension{u".TXT"};

// NB: Don't include UI headers to keep this code unit testable.
using option_t = std::pair<std::string_view, int32_t>;
using options_t = std::vector<option_t>;

options_t freqman_modulations = {
    {"AM", 0},
    {"NFM", 1},
    {"WFM", 2},
    {"SPEC", 3},
};

options_t freqman_bandwidths[5] = {
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
        // AMFM for Wefax-
        {"USB+FM", 5},  // Fixed RX demodul AM config Index 5 : USB+FM for Audio Weather fax (Wfax) tones.
    },
    {
        // SPEC -- TODO: these should be indexes.
        {"12k5", 12500},
        {"16k", 16000},
        {"25k", 25000},
        {"32k", 32000},
        {"50k", 50000},
        {"75k", 75000},
        {"100k", 100000},
        {"150k", 150000},
        {"250k", 250000},
        {"500k", 500000},
        {"600k", 600000},
        {"750k", 750000},
        {"1000k", 1000000},  // Max bandwith for recording in C16 (with fast SD card).
        {"1250k", 1250000},
        {"1500k", 1500000},
        {"1750k", 1750000},
        {"2000k", 2000000},
        {"2250k", 2250000},  // Max bandwith for recording in C8 (with fast SD card).
        {"2500k", 2500000},  // Here and up, LCD is ok, but M4 CPU drops samples.
        {"3000k", 3000000},
        {"3500k", 3500000},
        {"4000k", 4000000},
        {"4500k", 4500000},
        {"5000k", 5500000},
        {"5500k", 5500000},  // Max capture, needs /4 decimation, (22Mhz sampling ADC).
    }};

// TODO: these should be indexes.
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

// TODO: these should be indexes.
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

const option_t* find_by_index(const options_t& options, freqman_index_t index) {
    if (index < options.size())
        return &options[index];
    else
        return nullptr;
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

bool operator==(const freqman_entry& lhs, const freqman_entry& rhs) {
    auto equal = lhs.type == rhs.type &&
                 lhs.frequency_a == rhs.frequency_a &&
                 lhs.description == rhs.description &&
                 lhs.modulation == rhs.modulation &&
                 lhs.bandwidth == rhs.bandwidth;

    if (!equal)
        return false;

    if (lhs.type == freqman_type::Range) {
        equal = lhs.frequency_b == rhs.frequency_b &&
                lhs.step == rhs.step;
    } else if (lhs.type == freqman_type::HamRadio) {
        equal = lhs.frequency_b == rhs.frequency_b &&
                lhs.tone == rhs.tone;
    } else if (lhs.type == freqman_type::Repeater) {
        equal = lhs.frequency_b == rhs.frequency_b;
    }

    return equal;
}

std::string freqman_entry_get_modulation_string(freqman_index_t modulation) {
    if (auto opt = find_by_index(freqman_modulations, modulation))
        return (std::string)opt->first;
    return {};
}

std::string freqman_entry_get_bandwidth_string(freqman_index_t modulation, freqman_index_t bandwidth) {
    if (modulation < freqman_modulations.size()) {
        if (auto opt = find_by_index(freqman_bandwidths[modulation], bandwidth))
            return (std::string)opt->first;
    }
    return {};
}

std::string freqman_entry_get_step_string(freqman_index_t step) {
    if (auto opt = find_by_index(freqman_steps, step))
        return (std::string)opt->first;
    return {};
}

std::string freqman_entry_get_step_string_short(freqman_index_t step) {
    if (auto opt = find_by_index(freqman_steps_short, step))
        return (std::string)opt->first;
    return {};
}

const std::filesystem::path get_freqman_path(const std::string& stem) {
    return freqman_dir / stem + freqman_extension;
}

bool create_freqman_file(const std::string& file_stem) {
    auto fs_error = make_new_file(get_freqman_path(file_stem));
    return fs_error.ok();
}

bool load_freqman_file(const std::string& file_stem, freqman_db& db, freqman_load_options options) {
    return parse_freqman_file(get_freqman_path(file_stem), db, options);
}

void delete_freqman_file(const std::string& file_stem) {
    delete_file(get_freqman_path(file_stem));
}

std::string pretty_string(const freqman_entry& entry, size_t max_length) {
    std::string str;

    switch (entry.type) {
        case freqman_type::Single:
            str = to_string_short_freq(entry.frequency_a) + "M: " + entry.description;
            break;
        case freqman_type::Range:
            str = to_string_rounded_freq(entry.frequency_a, 1) + "M-" +
                  to_string_rounded_freq(entry.frequency_b, 1) + "M: " + entry.description;
            break;
        case freqman_type::HamRadio:
            str = "R:" + to_string_rounded_freq(entry.frequency_a, 1) + "M,T:" +
                  to_string_rounded_freq(entry.frequency_b, 1) + "M: " + entry.description;
            break;
        case freqman_type::Repeater:
            str = "L:" + to_string_rounded_freq(entry.frequency_a, 1) + "M,T:" +
                  to_string_rounded_freq(entry.frequency_b, 1) + "M: " + entry.description;
            break;
        case freqman_type::Raw:
            str = entry.description;
            break;
        default:
            str = "UNK:" + entry.description;
            break;
    }

    // Truncate. '+' indicates if string has been truncated.
    if (str.size() > max_length)
        return str.substr(0, max_length - 1) + "+";

    return str;
}

std::string to_freqman_string(const freqman_entry& entry) {
    std::string serialized;
    serialized.reserve(0x80);

    // Append a key=value to the string.
    auto append_field = [&serialized](std::string_view name, std::string_view value) {
        if (!serialized.empty())
            serialized += ",";
        serialized += std::string{name} + "=" + std::string{value};
    };

    switch (entry.type) {
        case freqman_type::Single:
            append_field("f", to_string_dec_uint(entry.frequency_a));
            break;
        case freqman_type::Range:
            append_field("a", to_string_dec_uint(entry.frequency_a));
            append_field("b", to_string_dec_uint(entry.frequency_b));

            if (is_valid(entry.step))
                append_field("s", freqman_entry_get_step_string_short(entry.step));
            break;
        case freqman_type::HamRadio:
            append_field("r", to_string_dec_uint(entry.frequency_a));
            append_field("t", to_string_dec_uint(entry.frequency_b));

            if (is_valid(entry.tone))
                append_field("c", tonekey::tone_key_value_string(entry.tone));
            break;
        case freqman_type::Repeater:
            append_field("l", to_string_dec_uint(entry.frequency_a));
            append_field("t", to_string_dec_uint(entry.frequency_b));
            break;
        case freqman_type::Raw:
            return entry.description;
        default:
            return {};
    };

    if (is_valid(entry.modulation) && entry.modulation < freqman_modulations.size()) {
        append_field("m", freqman_entry_get_modulation_string(entry.modulation));

        if (is_valid(entry.bandwidth) && (unsigned)entry.bandwidth < freqman_bandwidths[entry.modulation].size())
            append_field("bw", freqman_entry_get_bandwidth_string(entry.modulation, entry.bandwidth));
    }

    if (entry.description.size() > 0)
        append_field("d", entry.description);

    serialized.shrink_to_fit();
    return serialized;
}

freqman_index_t parse_tone_key(std::string_view value) {
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

    return static_cast<freqman_index_t>(tonekey::tone_key_index_by_value(tone_freq));
}

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
            entry.tone = parse_tone_key(value);
        } else if (key == "d") {
            entry.description = trim(value).substr(0, freqman_max_desc_size);
        } else if (key == "f") {
            entry.type = freqman_type::Single;
            parse_int(value, entry.frequency_a);
        } else if (key == "m") {
            entry.modulation = find_by_name(freqman_modulations, value);
        } else if (key == "r") {  // HamRadio relay receive freq
            entry.type = freqman_type::HamRadio;
            parse_int(value, entry.frequency_a);
        } else if (key == "l") {  // Portapack Repeater mode listen freq. Used as a single freq if Repeater mode isn't active
            entry.type = freqman_type::Repeater;
            parse_int(value, entry.frequency_a);
        } else if (key == "s") {
            entry.step = find_by_name(freqman_steps_short, value);
        } else if (key == "t") {  // Tx freq: scanned as a single freq in HamRadio mode, used as TX freq in Repeater mode and ignored by the scanner
            parse_int(value, entry.frequency_b);
        }
    }
    return is_valid(entry);
}

bool parse_freqman_file(const fs::path& path, freqman_db& db, freqman_load_options options) {
    FreqmanDB freqman_db;
    freqman_db.set_read_raw(false);  // Don't return malformed lines.
    if (!freqman_db.open(path))
        return false;

    // Attempt to avoid a re-alloc if possible.
    db.clear();
    db.reserve(freqman_db.entry_count());

    for (auto entry : freqman_db) {
        // Filter by entry type.
        if (entry.type == freqman_type::Unknown ||
            (entry.type == freqman_type::Single && !options.load_freqs) ||
            (entry.type == freqman_type::Range && !options.load_ranges) ||
            (entry.type == freqman_type::HamRadio && !options.load_hamradios) ||
            (entry.type == freqman_type::Repeater && !options.load_repeaters)) {
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

bool is_valid(const freqman_entry& entry) {
    // No valid frequency combination was set.
    if (entry.type == freqman_type::Unknown)
        return false;

    // Frequency A must be set for all types
    if (entry.frequency_a == 0)
        return false;

    // Frequency B must be set for type Range or HamRadio or Repeater
    if (entry.type == freqman_type::Range || entry.type == freqman_type::HamRadio || entry.type == freqman_type::Repeater) {
        if (entry.frequency_b == 0)
            return false;
    }

    // Ranges should have frequencies A <= B.
    if (entry.type == freqman_type::Range) {
        if (entry.frequency_a > entry.frequency_b)
            return false;
    }

    // TODO: Consider additional validation:
    // - Tone only on HamRadio.
    // - Step only on Range
    // - Fail on failed parse_int.
    // - Fail if bandwidth set before modulation.

    return true;
}

/* FreqmanDB ***********************************/

bool FreqmanDB::open(const std::filesystem::path& path, bool create) {
    auto result = FileWrapper::open(path, create);
    if (!result)
        return false;

    wrapper_ = *std::move(result);
    return true;
}

void FreqmanDB::close() {
    wrapper_.reset();
}

freqman_entry FreqmanDB::operator[](Index index) const {
    auto length = wrapper_->line_length(index);
    auto line_text = wrapper_->get_text(index, 0, length);

    if (line_text) {
        freqman_entry entry;
        if (parse_freqman_entry(*line_text, entry))
            return entry;
        else if (read_raw_) {
            entry.type = freqman_type::Raw;
            entry.description = trim(*line_text).substr(0, freqman_max_desc_size);
            return entry;
        }
    }

    return {};
}

void FreqmanDB::insert_entry(Index index, const freqman_entry& entry) {
    index = clip<uint32_t>(index, 0u, entry_count());
    wrapper_->insert_line(index);
    replace_entry(index, entry);
}

void FreqmanDB::append_entry(const freqman_entry& entry) {
    insert_entry(entry_count(), entry);
}

void FreqmanDB::replace_entry(Index index, const freqman_entry& entry) {
    auto range = wrapper_->line_range(index);
    if (!range)
        return;

    // Don't overwrite the '\n'.
    range->end--;
    wrapper_->replace_range(*range, to_freqman_string(entry));
}

void FreqmanDB::delete_entry(Index index) {
    wrapper_->delete_line(index);
}

bool FreqmanDB::delete_entry(const freqman_entry& entry) {
    auto it = find_entry(entry);
    if (it == end())
        return false;

    delete_entry(it.index());
    return true;
}

FreqmanDB::iterator FreqmanDB::find_entry(const freqman_entry& entry) {
    return find_entry([&entry](const auto& other) {
        return entry == other;
    });
}

uint32_t FreqmanDB::entry_count() const {
    // FileWrapper always presents a single line even for empty files.
    return empty() ? 0u : wrapper_->line_count();
}

bool FreqmanDB::empty() const {
    // FileWrapper always presents a single line even for empty files.
    // A DB is only really empty if the file size is 0.
    return !wrapper_ || wrapper_->size() == 0;
}
