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

#ifndef __FREQMAN_DB_H__
#define __FREQMAN_DB_H__

#include "file.hpp"
#include "file_wrapper.hpp"
#include "utility.hpp"

#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

/* Defined in freqman_db.cpp */
extern const std::filesystem::path freqman_dir;
extern const std::filesystem::path freqman_extension;

using freqman_index_t = uint8_t;
constexpr freqman_index_t freqman_invalid_index = static_cast<freqman_index_t>(-1);

/* Returns true if the value is not invalid_index. */
constexpr bool is_valid(freqman_index_t index) {
    return index != freqman_invalid_index;
}

/* Returns true if the value is invalid_index. */
constexpr bool is_invalid(freqman_index_t index) {
    return index == freqman_invalid_index;
}

/* Gets the full path for a given file stem (no extension). */
const std::filesystem::path get_freqman_path(const std::string& stem);

enum class freqman_type : uint8_t {
    Single,    // f=
    Range,     // a=,b=
    HamRadio,  // r=,t=
    Raw,       // line content in description
    Unknown,
};

/* Tables for next round of changes.
 * // TODO: attempt to consolidate all of these values
 * // and settings into a single location.
 * // Should tone_keys get consolidated here too?
 * enum class freqman_modulation : uint8_t {
    AM,
    NFM,
    WFM,
    SPEC,
    Unknown,
 * };
 *
 * struct freqman_modulation_info {
 *     freqman_modulation modulation;
 *     std::string_view name;
 * };
 *
 * constexpr std::array<freqman_modulation_info, 5> freqman_modulations = {
    freqman_modulation_info{ freqman_modulation::AM, "AM" },
    freqman_modulation_info{ freqman_modulation::NFM, "NFM" },
    freqman_modulation_info{ freqman_modulation::WFM, "WFM" },
    freqman_modulation_info{ freqman_modulation::SPEC, "SPEC" },
    freqman_modulation_info{ freqman_modulation::Unknown, "Unknown" }
 * };
 * static_assert(std::size(freqman_modulations) == (size_t)freqman_modulation::Unknown + 1);
 *
 * enum class freqman_step : uint8_t {
    _100Hz,
    _1kHz,
    _5kHz,
    _6_25kHz,
    _8_33kHz,
    _9kHz,
    _10kHz,
    _12_5kHz,
    _15kHz,
    _25kHz,
    _30kHz,
    _50kHz,
    _100kHz,
    _250kHz,
    _500kHz,
    _1MHz,
    Unknown,
 * };
 *
 * struct freqman_step_info {
 *     freqman_step step;
 *     std::string_view name;
 *     std::string_view display_name;
 *     uint32_t value;
 * };
 *
 * // TODO: FrequencyStepView should use this list.
 * constexpr std::array<freqman_step_info, 18> freqman_steps = {
    freqman_step_info{ freqman_step::_100Hz,   "0.1kHz",  "0.1kHz      ", 100 },
    freqman_step_info{ freqman_step::_1kHz,    "1kHz",    "1kHz        ", 1'000 },
    freqman_step_info{ freqman_step::_5kHz,    "5kHz",    "5kHz (SA AM)", 5'000 },
    freqman_step_info{ freqman_step::_6_25kHz, "6.25kHz", "6.25kHz(NFM)", 6'250 },
    freqman_step_info{ freqman_step::_8_33kHz, "8.33kHz", "8.33kHz(AIR)", 8'330 },
    freqman_step_info{ freqman_step::_9kHz,    "9kHz",    "9kHz (EU AM)", 9'000 },
    freqman_step_info{ freqman_step::_10kHz,   "10kHz",   "10kHz(US AM)", 10'000 },
    freqman_step_info{ freqman_step::_12_5kHz, "12.5kHz", "12.5kHz(NFM)", 12'500 },
    freqman_step_info{ freqman_step::_15kHz,   "15kHz",   "15kHz  (HFM)", 15'000 },
    freqman_step_info{ freqman_step::_25kHz,   "25kHz",   "25kHz   (N1)", 25'000 },
    freqman_step_info{ freqman_step::_30kHz,   "30kHz",   "30kHz (OIRT)", 30'000 },
    freqman_step_info{ freqman_step::_50kHz,   "50kHz",   "50kHz  (FM1)", 50'000 },
    freqman_step_info{ freqman_step::_100kHz,  "100kHz",  "100kHz (FM2)", 100'000 },
    freqman_step_info{ freqman_step::_250kHz,  "250kHz",  "250kHz  (N2)", 250'000 },
    freqman_step_info{ freqman_step::_500kHz,  "500kHz",  "500kHz (WFM)", 500'000 },
    freqman_step_info{ freqman_step::_1MHz,    "1MHz",    "1MHz        ", 1'000'000 },
    freqman_step_info{ freqman_step::Unknown,  "Unknown", "Unknown     ", 0 },
 * };
 * static_assert(std::size(freqman_steps) == (size_t)freqman_step::Unknown + 1);
*/

/* Freqman Entry *******************************/
struct freqman_entry {
    int64_t frequency_a{0};      // 'f=freq' or 'a=freq_start' or 'r=recv_freq'
    int64_t frequency_b{0};      // 'b=freq_end' or 't=tx_freq'
    std::string description{0};  // 'd=desc'
    freqman_type type{freqman_type::Unknown};
    freqman_index_t modulation{freqman_invalid_index};
    freqman_index_t bandwidth{freqman_invalid_index};
    freqman_index_t step{freqman_invalid_index};
    freqman_index_t tone{freqman_invalid_index};
};

// TODO: These shouldn't be exported.
std::string freqman_entry_get_modulation_string(freqman_index_t modulation);
std::string freqman_entry_get_bandwidth_string(freqman_index_t modulation, freqman_index_t bandwidth);
std::string freqman_entry_get_step_string(freqman_index_t step);
std::string freqman_entry_get_step_string_short(freqman_index_t step);

/* A reasonable maximum number of items to load from a freqman file.
 * Apps using freqman_db should be tested and this value tuned to
 * ensure app memory stability. */
constexpr size_t freqman_default_max_entries = 150;

struct freqman_load_options {
    /* Loads all entries when set to 0. */
    size_t max_entries{freqman_default_max_entries};
    bool load_freqs{true};
    bool load_ranges{true};
    bool load_hamradios{true};
};

using freqman_entry_ptr = std::unique_ptr<freqman_entry>;
using freqman_db = std::vector<freqman_entry_ptr>;

/* Gets a pretty string representation for an entry. */
std::string pretty_string(const freqman_entry& item, size_t max_length = 30);

/* Gets the freqman file representation for an entry. */
std::string to_freqman_string(const freqman_entry& entry);

bool parse_freqman_entry(std::string_view str, freqman_entry& entry);
bool parse_freqman_file(const std::filesystem::path& path, freqman_db& db, freqman_load_options options);

/* The tricky part of using the file directly is that there can be comments
 * and empty lines in the file. This messes up the 'count' calculation.
 * Either have to live with 'count' being an upper bound have the callers
 * know to expect that entries may be empty. */
// NB: This won't apply implicit mod/bandwidth.
// TODO: Reuse for parse_freqman_file
class FreqmanDB {
   public:
    class iterator {
       public:
        iterator(FreqmanDB& db, FileWrapper::Offset line)
            : db_{db}, line_{line} {}
        iterator& operator++() {
            line_++;
            return *this;
        }
        freqman_entry operator*() const {
            return db_[line_];
        }

        bool operator!=(const iterator& other) {
            return &db_ != &other.db_ || line_ != other.line_;
        }

       private:
        FreqmanDB& db_;
        FileWrapper::Line line_;
    };

    bool open(const std::filesystem::path& path);
    void close();
    freqman_entry operator[](FileWrapper::Line line) const;
    void insert_entry(const freqman_entry& entry, FileWrapper::Line line);
    void replace_entry(FileWrapper::Line line, const freqman_entry& entry);
    void delete_entry(FileWrapper::Line line);

    uint32_t entry_count() const;
    bool empty() const;

    iterator begin() {
        return {*this, 0};
    }
    iterator end() {
        return {*this, entry_count()};
    }

   private:
    std::unique_ptr<FileWrapper> wrapper_{};
};

#endif /* __FREQMAN_DB_H__ */