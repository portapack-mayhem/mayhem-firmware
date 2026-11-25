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

#include "dab/dab_crc.hpp"
#include "dab/dab_constants.hpp"

#include <cstdint>
#include <cstring>

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
