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

#include "dab/dab_prbs.hpp"

#include <cstdint>
#include <cstring>

TEST_CASE("prbs matches the reference sequence") {
    // The generator emits the feedback bit, not the bit shifted out. Emitting
    // the latter gives the same m-sequence delayed by nine bits: it still has
    // period 511 and 256 ones, so only a known vector catches the mistake.
    dab::PrbsGenerator g;
    CHECK(g.next_byte() == 0x07);
    CHECK(g.next_byte() == 0xBE);
    CHECK(g.next_byte() == 0x2E);
    CHECK(g.next_byte() == 0x64);
}

TEST_CASE("prbs opens with five zeros then three ones") {
    dab::PrbsGenerator g;
    for (int i = 0; i < 5; ++i)
        CHECK(g.next_bit() == 0);
    for (int i = 0; i < 3; ++i)
        CHECK(g.next_bit() == 1);
}

TEST_CASE("prbs sequence period is 511 bits") {
    dab::PrbsGenerator g;
    uint8_t first[511];
    for (auto& b : first)
        b = g.next_bit();

    for (size_t i = 0; i < 511; ++i)
        CHECK(g.next_bit() == first[i]);
}

TEST_CASE("prbs is not a constant sequence") {
    // Guards against a generator that never shifts.
    dab::PrbsGenerator g;
    int ones = 0;
    for (int i = 0; i < 511; ++i)
        ones += g.next_bit();

    // A maximal-length 9-bit LFSR emits 256 ones per period.
    CHECK(ones == 256);
}

TEST_CASE("descrambling is self inverse") {
    uint8_t data[64];
    uint8_t original[64];
    for (size_t i = 0; i < sizeof(data); ++i)
        data[i] = original[i] = static_cast<uint8_t>(i * 11 + 5);

    dab::prbs_descramble(data, sizeof(data));
    CHECK(std::memcmp(data, original, sizeof(data)) != 0);

    dab::prbs_descramble(data, sizeof(data));
    CHECK(std::memcmp(data, original, sizeof(data)) == 0);
}

TEST_CASE("descrambling an all-zero buffer yields the raw sequence") {
    uint8_t data[4]{};
    dab::prbs_descramble(data, sizeof(data));
    CHECK(data[0] == 0x07);
    CHECK(data[1] == 0xBE);
}

TEST_CASE("descramble restarts the sequence on every call") {
    uint8_t a[8]{};
    uint8_t b[8]{};
    dab::prbs_descramble(a, sizeof(a));
    dab::prbs_descramble(b, sizeof(b));
    CHECK(std::memcmp(a, b, sizeof(a)) == 0);
}
