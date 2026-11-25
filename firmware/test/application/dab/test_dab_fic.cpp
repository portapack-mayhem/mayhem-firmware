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

#include "dab/dab_fic.hpp"
#include "dab/dab_viterbi.hpp"
#include "dab/dab_protection.hpp"
#include "dab/dab_soft.hpp"

#include <cstdint>
#include <vector>

TEST_CASE("FIC sizes match the OFDM capacity they must fill") {
    // Three FIC symbols carry 3 * 1536 * 2 = 9216 bits per frame, split
    // across four codewords of 2304.
    CHECK(dab::kFicPuncturedBits * dab::kFicCodewordsPerFrame == 3 * 1536 * 2);
    CHECK(dab::kFicPuncturedBits == 2304);
    // Mother code is rate 1/4 over payload plus the 6-bit tail.
    CHECK(dab::kFicMotherBits == 4 * (dab::kFicPayloadBits + 6));
    // Four codewords carry the frame's 12 FIBs.
    CHECK(dab::kFicCodewordsPerFrame * 3 == 12);
}

TEST_CASE("the puncturing schedule consumes exactly the transmitted bits") {
    // 21 blocks of PI_16, 3 of PI_15, then a 12-bit tail.
    const size_t pi16_kept = dab::puncturing_weight(16);
    const size_t pi15_kept = dab::puncturing_weight(15);

    CHECK(pi16_kept == 24);
    CHECK(pi15_kept == 23);

    const size_t total = 21 * 4 * pi16_kept + 3 * 4 * pi15_kept + 12;
    CHECK(total == dab::kFicPuncturedBits);
}

TEST_CASE("depuncturing expands to the full mother codeword") {
    std::vector<int8_t> punctured(dab::kFicPuncturedBits, 64);
    std::vector<int8_t> out(dab::kFicMotherBits, 0);

    CHECK(dab::depuncture_fic(punctured.data(), out.data(), out.size()) == dab::kFicMotherBits);
}

TEST_CASE("depunctured erasures land exactly where bits were removed") {
    std::vector<int8_t> punctured(dab::kFicPuncturedBits, 64);
    std::vector<int8_t> out(dab::kFicMotherBits, 0);
    dab::depuncture_fic(punctured.data(), out.data(), out.size());

    size_t kept = 0;
    for (int8_t v : out)
        if (v != 0)
            ++kept;

    // Every transmitted bit must appear exactly once.
    CHECK(kept == dab::kFicPuncturedBits);
}

TEST_CASE("a FIC codeword survives encode, puncture and decode") {
    // Build a payload, encode at rate 1/4, puncture with the FIC schedule,
    // then depuncture and decode. This exercises the schedule against the
    // Viterbi decoder rather than only counting bits.
    std::vector<uint8_t> payload((dab::kFicPayloadBits + 7) / 8);
    uint32_t x = 12345;
    for (auto& b : payload) {
        x = x * 1103515245u + 12345u;
        b = static_cast<uint8_t>(x >> 16);
    }

    std::vector<uint8_t> coded(dab::kFicMotherBits);
    REQUIRE(dab::convolutional_encode(payload.data(), dab::kFicPayloadBits, coded.data()) == dab::kFicMotherBits);

    // Puncture: keep the bits the schedule keeps, in order.
    std::vector<int8_t> mother_soft(dab::kFicMotherBits);
    for (size_t i = 0; i < dab::kFicMotherBits; ++i)
        mother_soft[i] = coded[i] ? 64 : -64;

    // Derive the keep-mask by depuncturing a marker sequence.
    std::vector<int8_t> ones(dab::kFicPuncturedBits, 1);
    std::vector<int8_t> mask(dab::kFicMotherBits, 0);
    dab::depuncture_fic(ones.data(), mask.data(), mask.size());

    std::vector<int8_t> punctured;
    punctured.reserve(dab::kFicPuncturedBits);
    for (size_t i = 0; i < dab::kFicMotherBits; ++i)
        if (mask[i] != 0)
            punctured.push_back(mother_soft[i]);

    REQUIRE(punctured.size() == dab::kFicPuncturedBits);

    // Through the packed depuncturer the receiver runs, which fetches words.
    punctured.resize(dab::kFicPuncturedBits + 4, 0);
    std::vector<dab::ViterbiDecoder::Soft> restored(dab::kFicMotherBits / 4, 0);
    REQUIRE(dab::depuncture_fic_packed(punctured.data(), restored.data(), restored.size()) == dab::kFicMotherBits);

    dab::ViterbiDecoder decoder;
    std::vector<uint8_t> decoded((dab::kFicPayloadBits + 7) / 8, 0);
    REQUIRE(decoder.decode(restored.data(), dab::kFicMotherBits, decoded.data(), decoded.size()) == dab::kFicPayloadBits);

    for (size_t i = 0; i < payload.size(); ++i)
        REQUIRE(decoded[i] == payload[i]);
}

TEST_CASE("depuncturing rejects bad arguments") {
    std::vector<int8_t> punctured(dab::kFicPuncturedBits, 1);
    std::vector<int8_t> out(dab::kFicMotherBits, 0);

    CHECK(dab::depuncture_fic(nullptr, out.data(), out.size()) == 0);
    CHECK(dab::depuncture_fic(punctured.data(), nullptr, out.size()) == 0);
    CHECK(dab::depuncture_fic(punctured.data(), out.data(), 100) == 0);
}
