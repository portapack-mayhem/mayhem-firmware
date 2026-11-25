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

#include "dab/dab_reed_solomon.hpp"

#include <array>
#include <cstdint>
#include <cstring>

namespace {

using Block = std::array<uint8_t, dab::kRsN>;

Block make_block(uint32_t seed) {
    Block b{};
    uint32_t x = seed;
    for (size_t i = 0; i < dab::kRsK; ++i) {
        x = x * 1103515245u + 12345u;
        b[i] = static_cast<uint8_t>(x >> 16);
    }
    return b;
}

}  // namespace

TEST_CASE("clean block decodes with no corrections") {
    dab::ReedSolomon rs;
    auto block = make_block(1);
    rs.encode(block.data());

    auto copy = block;
    CHECK(rs.decode(copy.data()) == 0);
    CHECK(std::memcmp(copy.data(), block.data(), dab::kRsN) == 0);
}

TEST_CASE("corrects a single byte error anywhere in the block") {
    dab::ReedSolomon rs;
    auto original = make_block(2);
    rs.encode(original.data());

    for (size_t pos = 0; pos < dab::kRsN; ++pos) {
        auto block = original;
        block[pos] ^= 0x5A;

        const int corrected = rs.decode(block.data());
        REQUIRE(corrected == 1);
        REQUIRE(std::memcmp(block.data(), original.data(), dab::kRsN) == 0);
    }
}

TEST_CASE("corrects three scattered byte errors") {
    dab::ReedSolomon rs;
    auto original = make_block(3);
    rs.encode(original.data());

    auto block = original;
    block[5] ^= 0xFF;
    block[47] ^= 0x01;
    block[110] ^= 0x80;

    CHECK(rs.decode(block.data()) == 3);
    CHECK(std::memcmp(block.data(), original.data(), dab::kRsN) == 0);
}

TEST_CASE("corrects the full five byte error capability") {
    dab::ReedSolomon rs;
    auto original = make_block(4);
    rs.encode(original.data());

    auto block = original;
    const size_t positions[5] = {0, 20, 60, 100, 119};
    for (size_t p : positions)
        block[p] ^= 0xA5;

    CHECK(rs.decode(block.data()) == 5);
    CHECK(std::memcmp(block.data(), original.data(), dab::kRsN) == 0);
}

TEST_CASE("reports failure beyond its correction capability") {
    dab::ReedSolomon rs;
    auto original = make_block(5);
    rs.encode(original.data());

    auto block = original;
    const size_t positions[6] = {3, 17, 40, 71, 95, 118};
    for (size_t p : positions)
        block[p] ^= 0x3C;

    // Six errors exceed t=5. The decoder must say so rather than hand back
    // silently corrupted audio.
    CHECK(rs.decode(block.data()) == -1);
}

TEST_CASE("heavy corruption is rejected, not miscorrected") {
    dab::ReedSolomon rs;
    auto original = make_block(6);
    rs.encode(original.data());

    auto block = original;
    for (size_t i = 0; i < dab::kRsN; i += 2)
        block[i] ^= 0x77;

    CHECK(rs.decode(block.data()) == -1);
}

TEST_CASE("adjacent error bursts within capability are corrected") {
    dab::ReedSolomon rs;
    auto original = make_block(7);
    rs.encode(original.data());

    auto block = original;
    for (size_t i = 30; i < 35; ++i)
        block[i] ^= 0x11;

    CHECK(rs.decode(block.data()) == 5);
    CHECK(std::memcmp(block.data(), original.data(), dab::kRsN) == 0);
}

TEST_CASE("errors confined to the parity bytes are corrected") {
    dab::ReedSolomon rs;
    auto original = make_block(8);
    rs.encode(original.data());

    auto block = original;
    block[112] ^= 0x09;
    block[117] ^= 0x63;

    CHECK(rs.decode(block.data()) == 2);
    CHECK(std::memcmp(block.data(), original.data(), dab::kRsN) == 0);
}

TEST_CASE("decoder is reusable across many blocks") {
    dab::ReedSolomon rs;
    for (uint32_t seed = 10; seed < 40; ++seed) {
        auto original = make_block(seed);
        rs.encode(original.data());

        auto block = original;
        block[seed % dab::kRsN] ^= 0xC3;
        block[(seed * 7) % dab::kRsN] ^= 0x1F;

        const int corrected = rs.decode(block.data());
        REQUIRE(corrected >= 1);
        REQUIRE(std::memcmp(block.data(), original.data(), dab::kRsN) == 0);
    }
}
