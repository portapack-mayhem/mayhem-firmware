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

#include "dab/dab_common.hpp"

#include <cstdint>
#include <cstring>

/* ---- CRC-16 and firecode ---- */

TEST_CASE("crc16 ccitt against the published check vector") {
    // CRC-16/CCITT-FALSE of "123456789" is 0x29B1.
    const char* v = "123456789";
    CHECK(dab::crc16_ccitt(reinterpret_cast<const uint8_t*>(v), 9) == 0x29B1);
}

TEST_CASE("fib crc accepts a correctly stamped fib") {
    uint8_t fib[dab::kFibBytes]{};
    for (size_t i = 0; i < dab::kFibPayloadBytes; ++i)
        fib[i] = static_cast<uint8_t>(i * 7 + 3);

    dab::fib_crc_stamp(fib);
    CHECK(dab::fib_crc_ok(fib));
}

TEST_CASE("fib crc rejects a single flipped payload bit") {
    uint8_t fib[dab::kFibBytes]{};
    for (size_t i = 0; i < dab::kFibPayloadBytes; ++i)
        fib[i] = static_cast<uint8_t>(i * 7 + 3);
    dab::fib_crc_stamp(fib);

    // Every single-bit error in the payload must be caught.
    for (size_t byte = 0; byte < dab::kFibPayloadBytes; ++byte) {
        for (int bit = 0; bit < 8; ++bit) {
            fib[byte] ^= (1u << bit);
            CHECK_FALSE(dab::fib_crc_ok(fib));
            fib[byte] ^= (1u << bit);
        }
    }
    CHECK(dab::fib_crc_ok(fib));
}

TEST_CASE("fib crc rejects corruption of the crc field itself") {
    uint8_t fib[dab::kFibBytes]{};
    dab::fib_crc_stamp(fib);
    REQUIRE(dab::fib_crc_ok(fib));

    fib[30] ^= 0x01;
    CHECK_FALSE(dab::fib_crc_ok(fib));
    fib[30] ^= 0x01;
    fib[31] ^= 0x80;
    CHECK_FALSE(dab::fib_crc_ok(fib));
}

TEST_CASE("superframe firecode round trip") {
    uint8_t head[11]{};
    for (size_t i = 2; i < 11; ++i)
        head[i] = static_cast<uint8_t>(0xA0 + i);

    dab::firecode_stamp(head);
    CHECK(dab::firecode_ok(head));

    head[5] ^= 0x10;
    CHECK_FALSE(dab::firecode_ok(head));
}

TEST_CASE("firecode is sensitive across every protected byte") {
    uint8_t head[11]{};
    for (size_t i = 2; i < 11; ++i)
        head[i] = static_cast<uint8_t>(i * 13);
    dab::firecode_stamp(head);

    for (size_t byte = 2; byte < 11; ++byte) {
        head[byte] ^= 0x01;
        CHECK_FALSE(dab::firecode_ok(head));
        head[byte] ^= 0x01;
    }
    CHECK(dab::firecode_ok(head));
}

/* ---- Protection profiles and depuncturing ---- */

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
