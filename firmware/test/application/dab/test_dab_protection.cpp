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

#include "dab/dab_protection.hpp"

#include <cstdint>

using dab::ProtectionKind;
using dab::ProtectionProfile;

static ProtectionProfile eep_a(uint8_t level) {
    return ProtectionProfile{ProtectionKind::Eep, level, false};
}
static ProtectionProfile eep_b(uint8_t level) {
    return ProtectionProfile{ProtectionKind::Eep, level, true};
}

TEST_CASE("eep-a bitrates match the capacity arithmetic") {
    // 96 CU at EEP 3-A (rate 1/2) is the canonical 128 kbit/s DAB+ service.
    CHECK(dab::eep_bitrate(96, eep_a(3)) == 128000);
    // 96 CU at EEP 1-A (rate 1/4) carries half as much.
    CHECK(dab::eep_bitrate(96, eep_a(1)) == 64000);
    CHECK(dab::eep_bitrate(8, eep_a(4)) == 16000);
}

TEST_CASE("eep-b bitrates match the capacity arithmetic") {
    CHECK(dab::eep_bitrate(27, eep_b(1)) == 32000);
    CHECK(dab::eep_bitrate(54, eep_b(1)) == 64000);
    CHECK(dab::eep_bitrate(18, eep_b(3)) == 32000);
}

TEST_CASE("a size that is not a whole number of units is rejected") {
    // 95 CU is not divisible by the 6 CU/unit of EEP 3-A.
    CHECK(dab::eep_bitrate(95, eep_a(3)) == 0);
    CHECK(dab::eep_bitrate(0, eep_a(3)) == 0);
}

TEST_CASE("out of range protection levels are rejected") {
    CHECK(dab::eep_cu_per_unit(eep_a(0)) == 0);
    CHECK(dab::eep_cu_per_unit(eep_a(5)) == 0);
    CHECK(dab::eep_bitrate(96, eep_a(0)) == 0);
}

TEST_CASE("subchannel bits per CIF follow from the bitrate") {
    // 128 kbit/s over 96 ms is 12288 bits per frame, across 4 CIFs.
    CHECK(dab::eep_subchannel_bits(96, eep_a(3)) == 3072);
}

TEST_CASE("puncturing vectors have the standard's monotone weight ladder") {
    // PI_i keeps exactly i + 8 of the 32 mother-code bits.
    for (uint8_t i = 1; i <= 24; ++i)
        CHECK(dab::puncturing_weight(i) == i + 8);
}

TEST_CASE("puncturing vector bounds") {
    CHECK(dab::puncturing_vector(0) == 0);
    CHECK(dab::puncturing_vector(25) == 0);
    CHECK(dab::puncturing_vector(24) == 0xFFFFFFFFu);  // unpunctured mother code
    CHECK(dab::puncturing_vector(8) == 0xCCCCCCCCu);   // rate 1/2
    CHECK(dab::puncturing_vector(16) == 0xEEEEEEEEu);  // three of four kept
}

TEST_CASE("puncturing weight is strictly increasing") {
    for (uint8_t i = 2; i <= 24; ++i)
        CHECK(dab::puncturing_weight(i) > dab::puncturing_weight(i - 1));
}

TEST_CASE("depuncturing restores mother code positions with erasures") {
    // PI_8 (0xCCCCCCCC) keeps bits 0,1 of each group of 4.
    const int8_t punctured[8] = {10, 20, 30, 40, 50, 60, 70, 80};
    int8_t out[16]{};

    const size_t written = dab::depuncture(punctured, 8, 0xCCCCCCCCu, out, 16);
    REQUIRE(written == 16);

    CHECK(out[0] == 10);
    CHECK(out[1] == 20);
    CHECK(out[2] == 0);  // erased
    CHECK(out[3] == 0);  // erased
    CHECK(out[4] == 30);
    CHECK(out[5] == 40);
    CHECK(out[6] == 0);
    CHECK(out[7] == 0);
}

TEST_CASE("depuncturing the unpunctured vector is a copy") {
    const int8_t punctured[4] = {1, 2, 3, 4};
    int8_t out[4]{};
    CHECK(dab::depuncture(punctured, 4, 0xFFFFFFFFu, out, 4) == 4);
    for (int i = 0; i < 4; ++i)
        CHECK(out[i] == punctured[i]);
}

TEST_CASE("depuncturing rejects bad arguments") {
    int8_t out[4]{};
    const int8_t in[4] = {1, 2, 3, 4};
    CHECK(dab::depuncture(nullptr, 4, 0xFFFFFFFFu, out, 4) == 0);
    CHECK(dab::depuncture(in, 4, 0xFFFFFFFFu, nullptr, 4) == 0);
    CHECK(dab::depuncture(in, 4, 0, out, 4) == 0);
}

TEST_CASE("depuncturing never writes past its capacity") {
    const int8_t punctured[32] = {};
    int8_t out[8];
    for (auto& v : out)
        v = 0x7F;

    const size_t written = dab::depuncture(punctured, 32, 0xCCCCCCCCu, out, 8);
    CHECK(written <= 8);
}
