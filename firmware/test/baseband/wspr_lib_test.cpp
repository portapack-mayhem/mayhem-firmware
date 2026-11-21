/*
 * Copyright (C) 2026 Ivan
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

/*
 * Host tests for the WSPR protocol constants and the reference encoder.
 *
 * Every constant checked here comes from public protocol documentation, not
 * from any decoder source: the sync vector from the G4JNT "Coding Process"
 * (swharden.com/software/FSKview/wspr) cross-checked against the independent
 * copy at ganymedeham.blogspot.com (2015-04-24), packing formulas and the
 * polynomials from the same document plus the Hackaday "Careless WSPR" log.
 */

#include "wspr_encode.h"
#include "wspr_constants.h"

#include <doctest.h>

#include <cstring>
#include <vector>

namespace {

uint32_t bit_reverse_8(uint32_t v) {
    v = ((v & 0x55) << 1) | ((v >> 1) & 0x55);
    v = ((v & 0x33) << 2) | ((v >> 2) & 0x33);
    v = ((v & 0x0F) << 4) | ((v >> 4) & 0x0F);
    return v;
}

uint8_t parity32(uint32_t v) {
    v ^= v >> 16;
    v ^= v >> 8;
    v ^= v >> 4;
    v ^= v >> 2;
    v ^= v >> 1;
    return v & 1u;
}

// Independent re-implementation of the documented decode of the 50-bit payload,
// used to check that the packer lays the bits out as the documentation says.
struct Packed {
    uint32_t callsign;  // 28 bits
    uint32_t locator;   // 15 bits
    uint32_t power7;    // 7-bit field: type marker + dBm
};

Packed unpack50(const uint8_t bits[WSPR_SOURCE_BYTES]) {
    uint64_t payload = 0;
    for (int i = 0; i < WSPR_PAYLOAD_BITS; i++) {
        payload = (payload << 1) | ((bits[i >> 3] >> (7 - (i & 7))) & 1u);
    }
    Packed p;
    p.callsign = payload >> 22;
    p.locator = (payload >> 7) & 0x7FFFu;
    p.power7 = payload & 0x7Fu;
    return p;
}

}  // namespace

TEST_CASE("sync vector matches the published table") {
    // The table as published by G4JNT (rows of 42/42/42/36); the swharden
    // rendering carries a "0.0" typo for the final "0,0" pair, disambiguated
    // by the ganymedeham.blogspot.com copy. Both sources agreed on all 162.
    static const uint8_t published[WSPR_CODED_BITS] = {
        1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0,
        0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0,
        1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1,
        1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0};
    CHECK_EQ(sizeof(published), sizeof(wspr_sync_vector));
    CHECK_EQ(std::memcmp(published, wspr_sync_vector, sizeof(published)), 0);

    int ones = 0;
    for (int i = 0; i < WSPR_CODED_BITS; i++) {
        REQUIRE((wspr_sync_vector[i] == 0 || wspr_sync_vector[i] == 1));
        ones += wspr_sync_vector[i];
    }
    // 63 ones, as counted in both published copies.
    CHECK_EQ(ones, 63);
}

TEST_CASE("interleave is the documented bit-reversal walk") {
    // Worked bit-reversal examples from the G4JNT document: I=1 -> J=128 and
    // I=13 -> J=176 (which is >= 162 and therefore skipped).
    CHECK_EQ(bit_reverse_8(1), 128);
    CHECK_EQ(bit_reverse_8(13), 176);

    std::vector<uint8_t> seen(WSPR_CODED_BITS, 0);
    uint8_t s[WSPR_CODED_BITS];
    uint8_t d[WSPR_CODED_BITS];
    uint8_t back[WSPR_CODED_BITS];
    for (int i = 0; i < WSPR_CODED_BITS; i++) s[i] = (uint8_t)i;
    wspr_interleave(s, d);
    for (int i = 0; i < WSPR_CODED_BITS; i++) {
        REQUIRE(d[i] < WSPR_CODED_BITS);
        CHECK_EQ(seen[d[i]], 0);
        seen[d[i]] = 1;
    }
    // The first kept addresses in the documented walk: I=0 -> D[0]=S[0],
    // I=1 -> D[128]=S[1], I=2 -> D[64]=S[2], I=3 skipped (192), I=4 -> D[32]=S[3].
    CHECK_EQ(d[0], 0);
    CHECK_EQ(d[128], 1);
    CHECK_EQ(d[64], 2);
    CHECK_EQ(d[32], 3);
    // The compaction breaks self-inverse symmetry, so the inverse is its own
    // function and the pair round-trips.
    wspr_deinterleave(d, back);
    for (int i = 0; i < WSPR_CODED_BITS; i++) {
        CHECK_EQ(back[i], (uint8_t)i);
    }
}

TEST_CASE("callsign packing follows the G4JNT chain") {
    // Hand-computed from the documented chain for "K1ABC", which the packer
    // turns into " K1ABC" (third character must be a digit):
    // chars: space=36, K=20, '1'=1, A=10, B=11, C=12.
    // N = ((((36*36 + 20)*10 + 1)*27 + 0)*27 + 1)*27 + 2 = 259047992.
    const uint32_t n = (((((uint32_t)36 * 36 + 20) * 10 + 1) * 27 + 0) * 27 + 1) * 27 + 2;
    CHECK_EQ(n, 259047992u);

    uint8_t bits[WSPR_SOURCE_BYTES];
    REQUIRE(wspr_pack_message("K1ABC", "FN20", 37, bits));
    const Packed p = unpack50(bits);
    CHECK_EQ(p.callsign, n);
}

TEST_CASE("locator and power packing follow the G4JNT formulas") {
    // AA00 -> 32220, RR99 -> 179 (documented extremes; note the reversal).
    const uint32_t aa00 = (uint32_t)((179 - 10 * 0 - 0) * 180 + 10 * 0 + 0);
    const uint32_t rr99 = (uint32_t)((179 - 10 * 17 - 9) * 180 + 10 * 17 + 9);
    CHECK_EQ(aa00, 32220u);
    CHECK_EQ(rr99, 179u);

    // FN20: L1=F=5, L2=N=13, digits 2,0.
    const uint32_t fn20 = (uint32_t)((179 - 10 * 5 - 2) * 180 + 10 * 13 + 0);
    CHECK_EQ(fn20, 22990u);

    uint8_t bits[WSPR_SOURCE_BYTES];
    REQUIRE(wspr_pack_message("K1ABC", "FN20", 37, bits));
    const Packed p = unpack50(bits);
    CHECK_EQ(p.locator, fn20);
    // 7-bit power field = dBm + 64 (type marker in the high bits).
    CHECK_EQ(p.power7, 37u + 64u);
    // Tail bits beyond the 50 payload bits stay zero (the 31-bit flush). Byte
    // 6 holds payload bits 48-49 in its top two positions, so only the low
    // six bits must be clear.
    for (int i = 7; i < WSPR_SOURCE_BYTES; i++) CHECK_EQ(bits[i], 0);
    CHECK_EQ(bits[6] & 0x3Fu, 0);
}

TEST_CASE("packing rejects non-standard messages") {
    uint8_t bits[WSPR_SOURCE_BYTES];
    CHECK_FALSE(wspr_pack_message("", "FN20", 30, bits));        // empty
    CHECK_FALSE(wspr_pack_message("K1ABC2", "FN20", 30, bits));  // 7 chars, no room for fix
    CHECK_FALSE(wspr_pack_message("XX1XX", "ZZ99", 30, bits));   // bad grid letters
    CHECK_FALSE(wspr_pack_message("XX1XX", "FN20", 61, bits));   // dBm out of range
    CHECK_FALSE(wspr_pack_message("XX1XX", "FN20", -1, bits));   // dBm out of range
    CHECK_FALSE(wspr_pack_message("XX1XX", "FN2", 30, bits));    // locator too short
    // A 4-character call with a digit in third position is valid as-is.
    CHECK(wspr_pack_message("AB1D", "FN20", 30, bits));
    // A call whose third character is not a digit gains one leading space.
    CHECK(wspr_pack_message("K1ABC", "FN20", 30, bits));
    // Two leading spaces would be needed to place a digit third: not standard.
    CHECK_FALSE(wspr_pack_message("ABXDEF", "FN20", 30, bits));
}

TEST_CASE("encode produces valid tone sequences") {
    uint8_t tones[WSPR_CODED_BITS];
    REQUIRE(wspr_encode_message("K1ABC", "FN20", 37, tones));
    for (int i = 0; i < WSPR_CODED_BITS; i++) {
        REQUIRE(tones[i] < 4);
    }
    // Sync positions carry exactly the sync bit plus the coded bit: strip the
    // data bit and the sync vector must come back out.
    for (int i = 0; i < WSPR_CODED_BITS; i++) {
        CHECK_EQ((int)(tones[i] & 1u), (int)wspr_sync_vector[i]);
    }
}
