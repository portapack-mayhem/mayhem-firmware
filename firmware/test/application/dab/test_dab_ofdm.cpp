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

#include "dab/dab_ofdm.hpp"
#include "dab/dab_constants.hpp"
#include "dab/dab_freq_interleave.hpp"

#include <cmath>
#include <cstdint>
#include <vector>

namespace {

constexpr float kTwoPi = 6.283185307179586f;

/* The demapper puts a carrier at the symbol's mean level near 72; at 45
 * degrees each component is 72 * cos(45) ~ 51, less rounding. */
constexpr int kTypicalSoft = 40;

dab::Iq16 from_phase(float phase, float amplitude) {
    return dab::Iq16{static_cast<int16_t>(amplitude * std::cos(phase)),
                     static_cast<int16_t>(amplitude * std::sin(phase))};
}

/* A symbol whose guard is a genuine copy of the useful part's tail, with an
 * optional carrier offset applied across the whole thing. */
std::vector<dab::Iq16> make_symbol(float offset_hz) {
    std::vector<dab::Iq16> useful(dab::kFftSize);
    for (size_t n = 0; n < dab::kFftSize; ++n) {
        // An arbitrary but deterministic waveform.
        const float p = 0.013f * static_cast<float>(n) + 0.7f;
        useful[n] = from_phase(p, 4000.0f);
    }

    std::vector<dab::Iq16> symbol(dab::kSymbolSamples);
    // Guard is a copy of the tail of the useful part.
    for (size_t n = 0; n < dab::kGuardSamples; ++n)
        symbol[n] = useful[dab::kFftSize - dab::kGuardSamples + n];
    for (size_t n = 0; n < dab::kFftSize; ++n)
        symbol[dab::kGuardSamples + n] = useful[n];

    if (offset_hz != 0.0f)
        dab::correct_frequency_offset(symbol.data(), symbol.size(), -offset_hz);

    return symbol;
}

}  // namespace

TEST_CASE("null symbol is found in a synthetic frame") {
    // Loud everywhere except one null-length stretch.
    std::vector<dab::Iq16> frame(dab::kSymbolSamples * 8);
    for (auto& s : frame)
        s = dab::Iq16{3000, -2000};

    const size_t null_at = dab::kSymbolSamples * 3;
    for (size_t n = 0; n < dab::kNullSamples; ++n)
        frame[null_at + n] = dab::Iq16{5, -3};

    size_t offset = 0;
    uint32_t confidence = 0;
    REQUIRE(dab::find_null_symbol(frame.data(), frame.size(), offset, confidence));

    // Allow a little slack: neighbouring windows are nearly as quiet.
    CHECK(offset >= null_at - 16);
    CHECK(offset <= null_at + 16);
    CHECK(confidence > 150);
}

TEST_CASE("a flat buffer reports no null symbol") {
    std::vector<dab::Iq16> frame(dab::kSymbolSamples * 4);
    for (auto& s : frame)
        s = dab::Iq16{1500, 1500};

    size_t offset = 0;
    uint32_t confidence = 0;
    CHECK_FALSE(dab::find_null_symbol(frame.data(), frame.size(), offset, confidence));
}

TEST_CASE("a buffer shorter than a null symbol is refused") {
    std::vector<dab::Iq16> tiny(100);
    size_t offset = 0;
    uint32_t confidence = 0;
    CHECK_FALSE(dab::find_null_symbol(tiny.data(), tiny.size(), offset, confidence));
    CHECK_FALSE(dab::find_null_symbol(nullptr, 100000, offset, confidence));
}

TEST_CASE("zero frequency offset is measured as zero") {
    auto symbol = make_symbol(0.0f);
    const float est = dab::estimate_frequency_offset(symbol.data());
    CHECK(std::fabs(est) < 5.0f);
}

TEST_CASE("a known frequency offset is recovered") {
    // The H4M Pro's 0.5 ppm TCXO gives at most about 120 Hz at 240 MHz.
    for (float truth : {-120.0f, -40.0f, 40.0f, 120.0f}) {
        auto symbol = make_symbol(truth);
        const float est = dab::estimate_frequency_offset(symbol.data());
        CHECK(std::fabs(est - truth) < 15.0f);
    }
}

TEST_CASE("frequency correction cancels an applied offset") {
    auto symbol = make_symbol(100.0f);
    REQUIRE(std::fabs(dab::estimate_frequency_offset(symbol.data()) - 100.0f) < 15.0f);

    dab::correct_frequency_offset(symbol.data(), symbol.size(), 100.0f);
    CHECK(std::fabs(dab::estimate_frequency_offset(symbol.data())) < 15.0f);
}

TEST_CASE("dqpsk demapping recovers the four phase quadrants") {
    // Previous symbol at a fixed reference phase.
    const float base = 0.4f;
    dab::Iq16 prev[4];
    dab::Iq16 curr[4];
    for (int k = 0; k < 4; ++k)
        prev[k] = from_phase(base, 6000.0f);

    // Differential phases of +45, +135, -135, -45 degrees put one sample in
    // each quadrant, which is exactly what the two soft bits encode.
    const float deltas[4] = {kTwoPi / 8, 3 * kTwoPi / 8, -3 * kTwoPi / 8, -kTwoPi / 8};
    for (int k = 0; k < 4; ++k)
        curr[k] = from_phase(base + deltas[k], 6000.0f);

    int8_t soft[8];
    dab::dqpsk_demap(prev, curr, 4, soft);

    /* EN 300 401 section 14.5 maps bit 0 to +1, so a positive correlation is a
     * 0 bit and the soft value -- positive means 1 -- comes out negated. */
    CHECK(soft[0] < 0);  // +45: real +, imag + -> bits 0, 0
    CHECK(soft[4] < 0);
    CHECK(soft[1] > 0);  // +135: real -, imag + -> bits 1, 0
    CHECK(soft[5] < 0);
    CHECK(soft[2] > 0);  // -135: real -, imag - -> bits 1, 1
    CHECK(soft[6] > 0);
    CHECK(soft[3] < 0);  // -45: real +, imag - -> bits 0, 1
    CHECK(soft[7] > 0);
}

TEST_CASE("the demapped sign follows the standard's bit mapping") {
    /* A carrier whose differential phase is +45 degrees carries bits 0 and 0.
     * If this assertion is inverted the decoder is handed the complement of
     * the transmitted sequence, which is not a codeword, and no FIB CRC can
     * ever pass -- a failure that only shows up on air. */
    dab::Iq16 prev[1] = {from_phase(0.0f, 6000.0f)};
    dab::Iq16 curr[1] = {from_phase(kTwoPi / 8, 6000.0f)};

    int8_t soft[2];
    dab::dqpsk_demap(prev, curr, 1, soft);

    CHECK(soft[0] < 0);
    CHECK(soft[1] < 0);
}

TEST_CASE("dqpsk soft output is independent of absolute phase") {
    // The same differential phase must decode identically wherever it sits.
    int8_t a[2];
    int8_t b[2];

    dab::Iq16 prev_a[1] = {from_phase(0.0f, 6000.0f)};
    dab::Iq16 curr_a[1] = {from_phase(kTwoPi / 8, 6000.0f)};
    dab::dqpsk_demap(prev_a, curr_a, 1, a);

    dab::Iq16 prev_b[1] = {from_phase(2.5f, 6000.0f)};
    dab::Iq16 curr_b[1] = {from_phase(2.5f + kTwoPi / 8, 6000.0f)};
    dab::dqpsk_demap(prev_b, curr_b, 1, b);

    CHECK((a[0] > 0) == (b[0] > 0));
    CHECK((a[1] > 0) == (b[1] > 0));
    CHECK(a[0] != 0);
}

TEST_CASE("a faded carrier yields lower confidence than a strong one") {
    // Both carriers must be demapped in the same call: confidence is relative
    // within a symbol, which is what the receiver actually compares.
    // A 12 dB fade still decodes, but with much less weight.
    dab::Iq16 prev[2] = {from_phase(0.0f, 8000.0f), from_phase(0.0f, 2000.0f)};
    dab::Iq16 curr[2] = {from_phase(kTwoPi / 8, 8000.0f), from_phase(kTwoPi / 8, 2000.0f)};

    int8_t soft[4];
    dab::dqpsk_demap(prev, curr, 2, soft);

    CHECK(soft[1] != 0);
    CHECK((soft[0] > 0) == (soft[1] > 0));
    CHECK(std::abs(soft[1]) < std::abs(soft[0]));
    CHECK((soft[2] > 0) == (soft[3] > 0));
    CHECK(std::abs(soft[3]) < std::abs(soft[2]));
}

TEST_CASE("a deeply faded carrier gets almost no confidence") {
    // 28 dB down: the Viterbi decoder should be told "almost no information"
    // rather than handed a confident guess from noise.
    dab::Iq16 prev[2] = {from_phase(0.0f, 8000.0f), from_phase(0.0f, 300.0f)};
    dab::Iq16 curr[2] = {from_phase(kTwoPi / 8, 8000.0f), from_phase(kTwoPi / 8, 300.0f)};

    int8_t soft[4];
    dab::dqpsk_demap(prev, curr, 2, soft);

    CHECK(std::abs(soft[0]) > kTypicalSoft);
    CHECK(std::abs(soft[1]) * 16 < std::abs(soft[0]));
}

TEST_CASE("strong signals do not overflow into a wrong sign") {
    // Near full-scale input: the intermediate products exceed int32.
    dab::Iq16 prev[1] = {from_phase(0.0f, 32000.0f)};
    dab::Iq16 curr[1] = {from_phase(kTwoPi / 8, 32000.0f)};

    int8_t soft[2];
    dab::dqpsk_demap(prev, curr, 1, soft);

    // +45 degrees is bits 0,0, so both soft values are negative and large.
    CHECK(soft[0] < 0);
    CHECK(soft[1] < 0);
    CHECK(std::abs(soft[0]) > kTypicalSoft);
}

TEST_CASE("an all-zero symbol yields zero confidence, not noise") {
    dab::Iq16 prev[4]{};
    dab::Iq16 curr[4]{};
    int8_t soft[8];
    for (auto& v : soft)
        v = 99;

    dab::dqpsk_demap(prev, curr, 4, soft);
    for (int8_t v : soft)
        CHECK(v == 0);
}

TEST_CASE("dqpsk demapping refuses null buffers") {
    dab::Iq16 buf[1]{};
    int8_t soft[2]{};
    dab::dqpsk_demap(nullptr, buf, 1, soft);
    dab::dqpsk_demap(buf, nullptr, 1, soft);
    dab::dqpsk_demap(buf, buf, 1, nullptr);
    CHECK(soft[0] == 0);
}

TEST_CASE("carrier extraction picks 1536 distinct bins in interleaved order") {
    std::vector<dab::Iq16> bins(dab::kFftSize);
    for (size_t n = 0; n < dab::kFftSize; ++n)
        bins[n] = dab::Iq16{static_cast<int16_t>(n), static_cast<int16_t>(-static_cast<int>(n))};

    std::vector<dab::Iq16> carriers(dab::kCarriers);
    dab::extract_carriers(bins.data(), carriers.data());

    const int16_t* map = dab::frequency_map();
    for (size_t k = 0; k < dab::kCarriers; ++k) {
        const size_t expected_bin = dab::carrier_to_fft_bin(map[k]);
        CHECK(carriers[k].i == static_cast<int16_t>(expected_bin));
    }
}

TEST_CASE("carrier extraction never selects the DC bin") {
    std::vector<dab::Iq16> bins(dab::kFftSize);
    bins[0] = dab::Iq16{32000, 32000};  // a marker at DC
    for (size_t n = 1; n < dab::kFftSize; ++n)
        bins[n] = dab::Iq16{1, 1};

    std::vector<dab::Iq16> carriers(dab::kCarriers);
    dab::extract_carriers(bins.data(), carriers.data());

    for (size_t k = 0; k < dab::kCarriers; ++k)
        CHECK(carriers[k].i == 1);
}

TEST_CASE("integer frequency correction cancels a known offset") {
    // The integer corrector must match what the float reference undoes,
    // because at 120 Hz an uncorrected offset rotates DQPSK by about 54
    // degrees over a symbol and breaks the decision boundaries.
    for (float truth : {-120.0f, -40.0f, 40.0f, 120.0f}) {
        auto symbol = make_symbol(truth);
        REQUIRE(std::fabs(dab::estimate_frequency_offset(symbol.data()) - truth) < 15.0f);

        dab::FrequencyCorrector corrector;
        corrector.set_offset(truth);
        corrector.apply(symbol.data(), symbol.size());

        CHECK(std::fabs(dab::estimate_frequency_offset(symbol.data())) < 15.0f);
    }
}

TEST_CASE("a zero offset leaves the corrector inactive and the samples alone") {
    auto symbol = make_symbol(0.0f);
    auto original = symbol;

    dab::FrequencyCorrector corrector;
    corrector.set_offset(0.0f);
    CHECK_FALSE(corrector.active());

    corrector.apply(symbol.data(), symbol.size());
    for (size_t n = 0; n < symbol.size(); n++) {
        CHECK(symbol[n].i == original[n].i);
        CHECK(symbol[n].q == original[n].q);
    }
}

TEST_CASE("correction phase continues across successive calls") {
    // Applying to a symbol in two halves must equal applying in one go,
    // otherwise the correction would restart every symbol and drift.
    auto one_go = make_symbol(90.0f);
    auto in_halves = one_go;

    dab::FrequencyCorrector a;
    a.set_offset(90.0f);
    a.apply(one_go.data(), one_go.size());

    dab::FrequencyCorrector b;
    b.set_offset(90.0f);
    const size_t half = in_halves.size() / 2;
    b.apply(in_halves.data(), half);
    b.apply(in_halves.data() + half, in_halves.size() - half);

    for (size_t n = 0; n < one_go.size(); n++) {
        REQUIRE(one_go[n].i == in_halves[n].i);
        REQUIRE(one_go[n].q == in_halves[n].q);
    }
}

TEST_CASE("correction refuses a null buffer") {
    dab::FrequencyCorrector c;
    c.set_offset(100.0f);
    c.apply(nullptr, 16);
    CHECK(c.active());
}

TEST_CASE("reset clears the correction") {
    dab::FrequencyCorrector c;
    c.set_offset(100.0f);
    REQUIRE(c.active());
    c.reset();
    CHECK_FALSE(c.active());
}
