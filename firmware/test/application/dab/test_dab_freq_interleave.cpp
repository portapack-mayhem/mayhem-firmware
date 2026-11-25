/*
 * Copyright (C) 2026 Mayhem firmware contributors
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

#include "doctest.h"

#include "dab/dab_freq_interleave.hpp"
#include "dab/dab_constants.hpp"

#include <cstdint>
#include <set>

TEST_CASE("frequency map has one entry per active carrier") {
    int16_t map[dab::kCarriers];
    CHECK(dab::build_frequency_map(map, dab::kCarriers) == dab::kCarriers);
}

TEST_CASE("frequency map rejects a wrong-sized buffer") {
    int16_t map[8];
    CHECK(dab::build_frequency_map(map, 8) == 0);
    CHECK(dab::build_frequency_map(nullptr, dab::kCarriers) == 0);
}

TEST_CASE("frequency map is a permutation of the active carriers") {
    const int16_t* map = dab::frequency_map();

    std::set<int16_t> seen;
    for (size_t i = 0; i < dab::kCarriers; ++i) {
        const int16_t c = map[i];
        CHECK(c >= -768);
        CHECK(c <= 768);
        CHECK(c != 0);  // DC carrier is never used
        seen.insert(c);
    }
    CHECK(seen.size() == dab::kCarriers);
}

TEST_CASE("frequency map actually interleaves") {
    // A map that came out in natural order would decode nothing on air.
    const int16_t* map = dab::frequency_map();
    int adjacent_in_order = 0;
    for (size_t i = 1; i < dab::kCarriers; ++i) {
        if (map[i] == map[i - 1] + 1)
            ++adjacent_in_order;
    }
    CHECK(adjacent_in_order < 100);
}

TEST_CASE("carrier to fft bin maps the spectrum around DC") {
    CHECK(dab::carrier_to_fft_bin(1) == 1);
    CHECK(dab::carrier_to_fft_bin(768) == 768);
    CHECK(dab::carrier_to_fft_bin(-1) == 2047);
    CHECK(dab::carrier_to_fft_bin(-768) == 1280);
}

TEST_CASE("every mapped carrier lands on a distinct fft bin") {
    const int16_t* map = dab::frequency_map();
    std::set<size_t> bins;
    for (size_t i = 0; i < dab::kCarriers; ++i)
        bins.insert(dab::carrier_to_fft_bin(map[i]));
    CHECK(bins.size() == dab::kCarriers);
}

TEST_CASE("cached map matches a freshly built one") {
    int16_t fresh[dab::kCarriers];
    dab::build_frequency_map(fresh, dab::kCarriers);
    const int16_t* cached = dab::frequency_map();
    for (size_t i = 0; i < dab::kCarriers; ++i)
        CHECK(fresh[i] == cached[i]);
}
