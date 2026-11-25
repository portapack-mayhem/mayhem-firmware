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

#include "dab/dab_blocks.hpp"
#include "dab/dab_constants.hpp"

#include <cstring>

TEST_CASE("dab mode I constants") {
    CHECK(dab::kSampleRate == 2048000);
    CHECK(dab::kCarriers == 1536);
    CHECK(dab::kFftSize == 2048);
    CHECK(dab::kGuardSamples == 504);
    CHECK(dab::kSymbolSamples == 2552);
    CHECK(dab::kNullSamples == 2656);
    CHECK(dab::kSymbolsPerFrame == 76);
    CHECK(dab::kFicSymbols == 3);
    CHECK(dab::kMscSymbols == 72);
    CHECK(dab::kCifsPerFrame == 4);
    CHECK(dab::kCuPerCif == 864);
    CHECK(dab::kBitsPerCif == 55296);
    CHECK(dab::kFibsPerFrame == 12);
    CHECK(dab::kFibBytes == 32);
    CHECK(dab::kFrameDurationMs == 96);
}

TEST_CASE("band III block table") {
    const auto blocks = dab::band_iii_blocks();
    REQUIRE(blocks.count == 41);

    CHECK(std::strcmp(blocks.items[0].name, "5A") == 0);
    CHECK(blocks.items[0].frequency_hz == 174928000);

    CHECK(std::strcmp(blocks.items[blocks.count - 1].name, "13F") == 0);
    CHECK(blocks.items[blocks.count - 1].frequency_hz == 239200000);

    // Frequencies must be strictly ascending so a sweep can walk the table in order.
    for (size_t i = 1; i < blocks.count; ++i) {
        CHECK(blocks.items[i].frequency_hz > blocks.items[i - 1].frequency_hz);
    }
}

TEST_CASE("band III includes the national N blocks") {
    CHECK(dab::find_block("10N") != nullptr);
    CHECK(dab::find_block("11N") != nullptr);
    CHECK(dab::find_block("12N") != nullptr);
    CHECK(dab::find_block("10N")->frequency_hz == 210096000);
}

TEST_CASE("L-band block table") {
    const auto blocks = dab::l_band_blocks();
    REQUIRE(blocks.count == 23);

    CHECK(std::strcmp(blocks.items[0].name, "LA") == 0);
    CHECK(blocks.items[0].frequency_hz == 1452960000);
    CHECK(std::strcmp(blocks.items[blocks.count - 1].name, "LW") == 0);

    for (size_t i = 1; i < blocks.count; ++i) {
        CHECK(blocks.items[i].frequency_hz > blocks.items[i - 1].frequency_hz);
    }
}

TEST_CASE("block lookup by name") {
    const auto* b = dab::find_block("12B");
    REQUIRE(b != nullptr);
    CHECK(b->frequency_hz == 225648000);

    CHECK(dab::find_block("99Z") == nullptr);
    CHECK(dab::find_block("") == nullptr);
    CHECK(dab::find_block(nullptr) == nullptr);
}

TEST_CASE("every block is spaced clear of its neighbour") {
    // DAB occupies 1.536 MHz; the raster must never place two blocks closer
    // than that or a scan would decode the same ensemble twice.
    const auto blocks = dab::band_iii_blocks();
    for (size_t i = 1; i < blocks.count; ++i) {
        const auto gap = blocks.items[i].frequency_hz - blocks.items[i - 1].frequency_hz;
        CHECK(gap >= 160000);
    }
}
