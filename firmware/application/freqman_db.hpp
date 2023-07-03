/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2023 gullradriel, Nilorea Studio Inc., Kyle Reed
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

#include "optional.hpp"
#include "file.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

using freqman_index_t = uint8_t;
constexpr freqman_invalid_index = static_cast<freqman_index_t>(-1);

constexpr bool is_valid(freqman_index_t index) {
    return index != freqman_invalid_index;
}

enum class freqman_type : uint8_t {
    Single,    // f=
    Range,     // a=,b=
    HamRadio,  // r=,t=
    Unknown,
};

enum class freqman_modulation : uint8_t {
    AM,
    NFM,
    WFM,
    SPEC,
    Unknown,
};

// TODO: Polymorphic?
struct freqman_entry {
    uint64_t frequency_a{0};     // 'f=freq' or 'a=freq_start' or 'r=recv_freq'
    uint64_t frequency_b{0};     // 'b=freq_end' or 't=tx_freq'
    std::string description{0};  // 'd=desc'
    freqman_type type{freqman_type::Unknown};
    freqman_modulation modulation{freqman_modulation::Unknown};
    freqman_bandwidth bandwidth{freqman_bandwidth::Unknown};
    freqman_step step{freqman_step::Unknown};
    freqman_tone tone{freqman_tone::Unknown};
};

using freqman_entry_ptr = std::unique_ptr<freqman_entry>;

bool parse_freqman_entry(std::string_view str, freqman_entry& entry);

class FreqmanDB {
   public:
    FreqmanDB();
    FreqmanDB(const FreqmanDB&) = delete;
    FreqmanDB(FreqmanDB&&) = delete;
    FreqmanDB& operator=(const FreqmanDB&) = delete;
    FreqmanDB& operator=(FreqmanDB&&) = delete;

    size_t size() const { return 0; };

   private:
    std::vector<freqman_entry_ptr> entries_;
};

#endif /* __FREQMAN_DB_H__ */