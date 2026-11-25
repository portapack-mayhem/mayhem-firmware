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

#include "dab/dab_soft.hpp"
#include "dab/dab_viterbi.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

namespace {

constexpr size_t kPayloadBits = 200;

std::vector<uint8_t> make_payload(size_t bits, uint32_t seed) {
    std::vector<uint8_t> data((bits + 7) / 8, 0);
    uint32_t x = seed;
    for (auto& b : data) {
        x = x * 1103515245u + 12345u;
        b = static_cast<uint8_t>(x >> 16);
    }
    return data;
}

std::vector<int8_t> to_soft(const std::vector<uint8_t>& coded_bits, int8_t confidence) {
    std::vector<int8_t> soft(coded_bits.size());
    for (size_t i = 0; i < coded_bits.size(); ++i)
        soft[i] = coded_bits[i] ? confidence : static_cast<int8_t>(-confidence);
    return soft;
}

/* The trellis input: one step word per four soft values, reduced to codes
 * (dab_soft.hpp), the step's first value in the low nibble. */
std::vector<dab::ViterbiDecoder::Soft> to_steps(const std::vector<int8_t>& soft) {
    std::vector<dab::ViterbiDecoder::Soft> steps(soft.size() / 4, 0);
    for (size_t i = 0; i < steps.size() * 4; ++i)
        steps[i / 4] = static_cast<dab::ViterbiDecoder::Soft>(steps[i / 4] | (dab::soft_code(soft[i]) << (4 * (i % 4))));
    return steps;
}

size_t bit_errors(const uint8_t* a, const uint8_t* b, size_t bits) {
    size_t errors = 0;
    for (size_t i = 0; i < bits; ++i) {
        const uint8_t x = (a[i >> 3] >> (7 - (i & 7))) & 1;
        const uint8_t y = (b[i >> 3] >> (7 - (i & 7))) & 1;
        if (x != y)
            ++errors;
    }
    return errors;
}

}  // namespace

TEST_CASE("generator polynomials produce the documented outputs") {
    // All-zero register must produce all-zero output.
    CHECK(dab::convolutional_output(0) == 0);
    // 133 and the fourth generator are identical, so bit 3 and bit 0 always agree.
    for (int reg = 0; reg < 128; ++reg) {
        const uint8_t out = dab::convolutional_output(static_cast<uint8_t>(reg));
        CHECK(((out >> 3) & 1) == (out & 1));
    }
}

TEST_CASE("encoder emits four bits per input bit plus the flush") {
    auto payload = make_payload(kPayloadBits, 1);
    std::vector<uint8_t> coded(4 * (kPayloadBits + 6));
    const size_t written = dab::convolutional_encode(payload.data(), kPayloadBits, coded.data());
    CHECK(written == 4 * (kPayloadBits + 6));
}

TEST_CASE("clean round trip is bit exact") {
    auto payload = make_payload(kPayloadBits, 42);
    std::vector<uint8_t> coded(4 * (kPayloadBits + 6));
    dab::convolutional_encode(payload.data(), kPayloadBits, coded.data());

    auto soft = to_soft(coded, 64);

    dab::ViterbiDecoder decoder;
    std::vector<uint8_t> out((kPayloadBits + 7) / 8, 0);
    const size_t decoded = decoder.decode(to_steps(soft).data(), soft.size(), out.data(), out.size());

    REQUIRE(decoded == kPayloadBits);
    CHECK(bit_errors(payload.data(), out.data(), kPayloadBits) == 0);
}

TEST_CASE("corrects scattered bit errors") {
    auto payload = make_payload(kPayloadBits, 7);
    std::vector<uint8_t> coded(4 * (kPayloadBits + 6));
    dab::convolutional_encode(payload.data(), kPayloadBits, coded.data());

    auto soft = to_soft(coded, 64);
    // Flip three well-separated coded bits; a K=7 rate-1/4 code absorbs these.
    soft[40] = static_cast<int8_t>(-soft[40]);
    soft[300] = static_cast<int8_t>(-soft[300]);
    soft[600] = static_cast<int8_t>(-soft[600]);

    dab::ViterbiDecoder decoder;
    std::vector<uint8_t> out((kPayloadBits + 7) / 8, 0);
    const size_t decoded = decoder.decode(to_steps(soft).data(), soft.size(), out.data(), out.size());

    REQUIRE(decoded == kPayloadBits);
    CHECK(bit_errors(payload.data(), out.data(), kPayloadBits) == 0);
}

TEST_CASE("erasures from depuncturing are tolerated") {
    auto payload = make_payload(kPayloadBits, 99);
    std::vector<uint8_t> coded(4 * (kPayloadBits + 6));
    dab::convolutional_encode(payload.data(), kPayloadBits, coded.data());

    auto soft = to_soft(coded, 64);
    // Erase every fourth coded bit, as puncturing to rate 1/3 would.
    for (size_t i = 3; i < soft.size(); i += 4)
        soft[i] = 0;

    dab::ViterbiDecoder decoder;
    std::vector<uint8_t> out((kPayloadBits + 7) / 8, 0);
    const size_t decoded = decoder.decode(to_steps(soft).data(), soft.size(), out.data(), out.size());

    REQUIRE(decoded == kPayloadBits);
    CHECK(bit_errors(payload.data(), out.data(), kPayloadBits) == 0);
}

TEST_CASE("a long burst of corruption is not silently accepted") {
    // Guards against a decoder that ignores its input and returns something
    // plausible regardless.
    auto payload = make_payload(kPayloadBits, 5);
    std::vector<uint8_t> coded(4 * (kPayloadBits + 6));
    dab::convolutional_encode(payload.data(), kPayloadBits, coded.data());

    auto soft = to_soft(coded, 64);
    for (size_t i = 100; i < 240; ++i)
        soft[i] = static_cast<int8_t>(-soft[i]);

    dab::ViterbiDecoder decoder;
    std::vector<uint8_t> out((kPayloadBits + 7) / 8, 0);
    decoder.decode(to_steps(soft).data(), soft.size(), out.data(), out.size());

    CHECK(bit_errors(payload.data(), out.data(), kPayloadBits) > 0);
}

TEST_CASE("decoder is reusable across blocks") {
    dab::ViterbiDecoder decoder;
    for (uint32_t seed = 1; seed <= 4; ++seed) {
        auto payload = make_payload(kPayloadBits, seed);
        std::vector<uint8_t> coded(4 * (kPayloadBits + 6));
        dab::convolutional_encode(payload.data(), kPayloadBits, coded.data());
        auto soft = to_soft(coded, 64);

        std::vector<uint8_t> out((kPayloadBits + 7) / 8, 0);
        const size_t decoded = decoder.decode(to_steps(soft).data(), soft.size(), out.data(), out.size());
        REQUIRE(decoded == kPayloadBits);
        CHECK(bit_errors(payload.data(), out.data(), kPayloadBits) == 0);
    }
}

TEST_CASE("a full FIC codeword decodes") {
    // FIC codewords carry 768 payload bits.
    constexpr size_t kFicBits = 768;
    auto payload = make_payload(kFicBits, 31337);
    std::vector<uint8_t> coded(4 * (kFicBits + 6));
    dab::convolutional_encode(payload.data(), kFicBits, coded.data());
    auto soft = to_soft(coded, 64);

    dab::ViterbiDecoder decoder;
    std::vector<uint8_t> out((kFicBits + 7) / 8, 0);
    const size_t decoded = decoder.decode(to_steps(soft).data(), soft.size(), out.data(), out.size());

    REQUIRE(decoded == kFicBits);
    CHECK(bit_errors(payload.data(), out.data(), kFicBits) == 0);
}

TEST_CASE("rejects malformed arguments") {
    dab::ViterbiDecoder decoder;
    const size_t coded = 4 * (kPayloadBits + 6);
    std::vector<dab::ViterbiDecoder::Soft> steps(coded / 4, 0x1111);
    std::vector<uint8_t> out(64, 0);

    CHECK(decoder.decode(nullptr, coded, out.data(), out.size()) == 0);
    CHECK(decoder.decode(steps.data(), coded, nullptr, out.size()) == 0);
    CHECK(decoder.decode(steps.data(), 0, out.data(), out.size()) == 0);
    CHECK(decoder.decode(steps.data(), 7, out.data(), out.size()) == 0);   // not a multiple of 4
    CHECK(decoder.decode(steps.data(), coded, out.data(), 1) == 0);  // output too small
}

TEST_CASE("rejects a payload beyond the supported bound") {
    dab::ViterbiDecoder decoder;
    const size_t too_many = dab::kViterbiMaxPayloadBits + 8;
    std::vector<dab::ViterbiDecoder::Soft> steps(too_many + 6, 0x1111);
    std::vector<uint8_t> out((too_many + 7) / 8, 0);
    CHECK(decoder.decode(steps.data(), 4 * (too_many + 6), out.data(), out.size()) == 0);
}
