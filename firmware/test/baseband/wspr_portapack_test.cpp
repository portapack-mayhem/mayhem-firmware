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
 * Host tests for the WSPR baseband glue, RING contract: the 352-row magnitude buffer
 * is a ring addressed by the absolute block-row counter, rows are overwritten in slot
 * order, the counter is published after the row bytes, and every consumer-facing
 * address is computed wrap-safe. A phase-continuous 4-FSK synthesis (loud enough that
 * each symbol's tone is the maximum of its row) must land in the physical slots the
 * ring equations promise — including across the ring seam and near the uint32 wrap.
 */

#ifndef DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#endif
#include <doctest.h>

// The glue header lives in firmware/baseband, which the standalone build has no
// -I for, so it is reached relatively.
#include "../../baseband/wspr_portapack.h"

#include "wspr_constants.h"
#include "wspr_encode.h"

#include <cmath>
#include <cstddef>
#include <cstring>
#include <vector>

namespace {

// Absolute bin of matrix column 0 (956 * 12000/8192 = 1400.39 Hz). The synthesis puts
// tone 0 ten bins above it, so symbol n is expected in column 10 + tones[n].
constexpr int kFirstBin = 956;
constexpr int kSynthBaseBin = kFirstBin + 10;

// Tone amplitude. An on-bin tone lands at byte 240 + 20*log10(amp^2/4) = 195, while
// unit-variance noise averages ~156 per bin and its loudest of a row's 150 bins ~172, so
// the tone clears even the worst noise byte by ~20 and the per-row maximum is reliably
// the tone.
constexpr float kToneAmp = 0.15f;

// Deterministic Gaussian noise so test runs are reproducible.
struct Noise {
    uint32_t state;
    explicit Noise(uint32_t seed)
        : state{seed ? seed : 1u} {}

    uint32_t next() {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }
    float uniform() { return (float)(next() >> 8) / (float)(1u << 24); }
    float gaussian() {
        float u1 = uniform();
        float u2 = uniform();
        if (u1 < 1e-9f) u1 = 1e-9f;
        return std::sqrt(-2.0f * std::log(u1)) * std::cos(6.2831853f * u2);
    }
};

// Synthesize `num_samples` of 12 kHz audio: the transmission starting at sample
// start_sample (0 = the first fed sample), tone n on absolute bin kSynthBaseBin +
// tones[n] with continuous phase across symbol boundaries, plus unit-variance noise.
void synthesize(const uint8_t* tones, uint32_t seed, size_t start_sample, size_t num_samples, std::vector<float>& out) {
    Noise noise{seed};
    out.assign(num_samples, 0.0f);

    const float two_pi = 6.2831853f;
    float phase = 0.0f;
    for (int n = 0; n < WSPR_CODED_BITS; n++) {
        const int bin = kSynthBaseBin + tones[n];
        const float omega = two_pi * (float)bin / (float)WSPR_SAMPLES_PER_SYMBOL;
        const size_t begin = start_sample + (size_t)n * WSPR_SAMPLES_PER_SYMBOL;
        if (begin + WSPR_SAMPLES_PER_SYMBOL > num_samples) break;
        for (int i = 0; i < WSPR_SAMPLES_PER_SYMBOL; i++) {
            out[begin + i] += kToneAmp * std::sin(phase + omega * (float)i);
        }
        // Continuous phase across symbols.
        phase = std::fmod(phase + omega * (float)WSPR_SAMPLES_PER_SYMBOL, two_pi);
    }
    for (size_t i = 0; i < out.size(); i++) {
        out[i] += noise.gaussian();
    }
}

// Column of the row's largest byte.
int row_argmax(const uint8_t* row) {
    int best_col = 0;
    for (int b = 1; b < WSPR_RX_NUM_BINS; b++) {
        if (row[b] > row[best_col]) best_col = b;
    }
    return best_col;
}

// The expected physical slot of absolute block-row br, wrap-safe (id modulo first).
uint32_t phys_slot(uint32_t br) {
    return br % (uint32_t)WSPR_RX_BLOCK_ROWS;
}

// Block-row id of symbol row n of one bank: interleaved stride under OSR2, plain
// rows under OSR1.
constexpr uint32_t kSymStride = WSPR_RX_TIME_OSR;
// Total completions when exactly one symbol period (plus the half-symbol bank-1 tail)
// of audio has been fed: OSR2's two banks alternate, OSR1 closes one row per symbol.
uint32_t completions_for_rows(uint32_t symbol_rows) {
    return WSPR_RX_TIME_OSR == 2 ? 2u * symbol_rows - 1u : symbol_rows;
}

}  // namespace

TEST_CASE("init and free set up and clear the glue") {
    // Null guards: no crash, no rows.
    CHECK_FALSE(wspr_portapack_init(nullptr));
    CHECK_FALSE(wspr_portapack_feed_sample(nullptr, 0.0f));
    wspr_portapack_free(nullptr);
    CHECK_EQ(wspr_portapack_total_br(nullptr), 0);
    CHECK_EQ(wspr_portapack_valid_rows(nullptr), 0);

    wspr_portapack_state_t state;
    REQUIRE(wspr_portapack_init(&state));
    CHECK_EQ(state.total_br, 0u);
    CHECK_EQ(state.bank0_samples_in_row, 0u);
    CHECK_FALSE(state.bank1_started);

    CHECK(wspr_portapack_matrix() != nullptr);
    CHECK_EQ(wspr_portapack_rows(), WSPR_RX_NUM_ROWS);
    CHECK_EQ(wspr_portapack_bins(), WSPR_RX_NUM_BINS);
    CHECK_EQ(wspr_portapack_bin0_hz(), WSPR_RX_BIN0_HZ);

    // A fresh app must not inherit magnitudes from a previous run.
    const std::vector<uint8_t> zeros(WSPR_RX_MATRIX_SIZE, 0);
    CHECK_EQ(std::memcmp(wspr_portapack_matrix(), zeros.data(), WSPR_RX_MATRIX_SIZE), 0);

    for (int i = 0; i < 3; i++) {
        CHECK_FALSE(wspr_portapack_feed_sample(&state, 0.0f));
    }
    CHECK_EQ(state.bank0_samples_in_row, 3u);
    CHECK_EQ(wspr_portapack_total_br(&state), 0u);
    CHECK_EQ(wspr_portapack_valid_rows(&state), 0u);

    // Nothing is heap-allocated; free just clears the state.
    wspr_portapack_free(&state);
    CHECK_EQ(state.total_br, 0u);
    CHECK_EQ(state.bank0_samples_in_row, 0u);
}

TEST_CASE("a full packet lands every tone in its ring slot") {
    uint8_t tones[WSPR_CODED_BITS];
    REQUIRE(wspr_encode_message("K1ABC", "FN20", 37, tones));

    std::vector<float> samples;
    synthesize(tones, 12345, 0, (size_t)WSPR_RX_NUM_ROWS * WSPR_SAMPLES_PER_SYMBOL, samples);

    wspr_portapack_state_t state;
    REQUIRE(wspr_portapack_init(&state));

    const size_t total = (size_t)WSPR_RX_NUM_ROWS * WSPR_SAMPLES_PER_SYMBOL;
    uint32_t completions = 0;
    for (size_t i = 0; i < total; i++) {
        if (wspr_portapack_feed_sample(&state, samples[i])) completions++;
    }

    // OSR2 interleaves two banks: 176 rows of audio close 2*176 - 1 block-rows (bank 1's
    // last row would run half a symbol past the fed audio and stays unclosed).
    const uint32_t expected_br = completions_for_rows(WSPR_RX_NUM_ROWS);
    CHECK_EQ(wspr_portapack_total_br(&state), expected_br);
    CHECK_EQ(completions, expected_br);
    CHECK_EQ(wspr_portapack_valid_rows(&state), expected_br);

    // Bank 0 rows are the even block-rows: symbol n sits in block-row 2n, which in the
    // ring is physical slot (2n % 352) — identical to the old window layout for the
    // first window, and the tone must be its row maximum in its own column.
    for (int n = 0; n < WSPR_CODED_BITS; n++) {
        const int col = 10 + tones[n];
        const uint8_t* row = wspr_portapack_row_br(kSymStride * (uint32_t)n);
        CAPTURE(n);
        CAPTURE(col);
        CHECK(row_argmax(row) >= col - 1);
        CHECK(row_argmax(row) <= col + 1);
    }

    // Bank 1 rows straddle two symbols: the argmax sits in one of the two adjacent
    // tone columns.
    if (WSPR_RX_TIME_OSR == 2) {
        for (int n = 0; n + 1 < WSPR_CODED_BITS; n++) {
            const int col_a = 10 + tones[n];
            const int col_b = 10 + tones[n + 1];
            const uint8_t* row = wspr_portapack_row_br(kSymStride * (uint32_t)n + 1u);
            const int am = row_argmax(row);
            CAPTURE(n);
            CAPTURE(am);
            const bool near_a = am >= col_a - 1 && am <= col_a + 1;
            const bool near_b = am >= col_b - 1 && am <= col_b + 1;
            CHECK((near_a || near_b));
        }
    }
}

TEST_CASE("the ring overwrites oldest rows in slot order") {
    wspr_portapack_state_t state;
    REQUIRE(wspr_portapack_init(&state));

    // Feed 3 windows' worth of silence-ish noise: total_br passes 2*352, every slot has
    // been overwritten at least once, counters keep moving, valid_rows saturates.
    Noise noise{999};
    const size_t total = 3u * WSPR_RX_BLOCK_ROWS * WSPR_SAMPLES_PER_SYMBOL;
    uint32_t completions = 0;
    for (size_t i = 0; i < total; i++) {
        if (wspr_portapack_feed_sample(&state, noise.gaussian())) completions++;
    }
    // OSR2 closes one block-row per half symbol: bank 0 every 8192 samples from 0,
    // bank 1 every 8192 from 4096, so 3 x 352 symbol periods minus bank 1's unclosed
    // final half-row yield exactly this count, and the counters never disagree.
    CHECK_EQ(completions, state.total_br);
    CHECK(completions >= 3u * WSPR_RX_BLOCK_ROWS - 2u);
    CHECK_EQ(wspr_portapack_valid_rows(&state), (uint32_t)WSPR_RX_BLOCK_ROWS);

    // Slot order: the newest row id (total_br - 1) must be in slot (total_br - 1) % 352,
    // i.e. right behind the write cursor. Mark a byte in the row the next completion
    // will overwrite, feed one row, and check exactly that slot changed and no other
    // slot's bytes moved more than quantization allows (all rows are noise, so compare
    // only the targeted slot for the exact new content and its neighbours for survival).
    uint32_t next_row = state.total_br;  // id of the next row to be closed
    uint8_t* target = (uint8_t*)wspr_portapack_row_br(next_row);
    const uint8_t marker = 123;
    *target = marker;
    REQUIRE(wspr_portapack_feed_sample(&state, noise.gaussian()) == false);
    // Half a row of samples closes bank 0's next row (OSR2: whichever bank's boundary
    // comes first closes SOME row; keep feeding until a completion).
    uint32_t guard = 0;
    while (state.total_br == next_row && guard++ < 2 * WSPR_SAMPLES_PER_SYMBOL) {
        wspr_portapack_feed_sample(&state, noise.gaussian());
    }
    REQUIRE(state.total_br == next_row + 1);
    CHECK_NE(*target, marker);
    CHECK_EQ(wspr_portapack_row_br(next_row), (const uint8_t*)target);
    // The physical slot of the row AFTER the overwritten one keeps its old address.
    CHECK_EQ(wspr_portapack_row_br(next_row + 1u),
             (const uint8_t*)wspr_portapack_matrix() +
                 (size_t)phys_slot(next_row + 1u) * WSPR_RX_NUM_BINS);
}

#if WSPR_RX_TIME_OSR == 2
TEST_CASE("bank 1 starts once per session and never re-arms") {
    wspr_portapack_state_t state;
    REQUIRE(wspr_portapack_init(&state));

    // Before the 4096-sample offset elapses, no bank-1 pass runs: after 4095 samples
    // bank 0 is mid-row, bank 1 has accumulated nothing.
    for (uint32_t i = 0; i < WSPR_RX_BANK1_OFFSET - 1; i++) {
        REQUIRE_FALSE(wspr_portapack_feed_sample(&state, 0.0f));
    }
    CHECK_FALSE(state.bank1_started);
    CHECK_EQ(state.bank1_samples_in_row, 0u);
    CHECK_EQ(state.bank0_samples_in_row, WSPR_RX_BANK1_OFFSET - 1);

    // The latch compares the PRE-increment sample count, so bank 1 starts
    // accumulating with the 4097th sample.
    CHECK_FALSE(wspr_portapack_feed_sample(&state, 0.0f));
    CHECK_FALSE(state.bank1_started);
    CHECK_FALSE(wspr_portapack_feed_sample(&state, 0.0f));
    CHECK(state.bank1_started);
    CHECK_EQ(state.bank1_samples_in_row, 1u);

    // Completion alternation: the first block-row completes at sample 8192 (bank 0 row
    // 0), the second at 12288 (bank 1 row 0), so after feeding 12288 samples total_br
    // is 2 and both completions map to the right parity.
    for (uint32_t i = 0; i < 2 * WSPR_SAMPLES_PER_SYMBOL - WSPR_RX_BANK1_OFFSET; i++) {
        wspr_portapack_feed_sample(&state, 0.0f);
    }
    // Completions at stream samples 8192 (bank 0 row 0, id 0), 12288 (bank 1 row 0,
    // id 1) and 16384 (bank 0 row 1, id 2).
    REQUIRE_EQ(state.total_br, 3u);
    // Slots 0 and 1 hold ids 0 and 1 — the physical layout starts interleaved exactly
    // as the ring equations promise (even id = bank 0, odd = bank 1).
    CHECK_EQ(wspr_portapack_row_br(0), wspr_portapack_matrix());
    CHECK_EQ(wspr_portapack_row_br(1), wspr_portapack_matrix() + WSPR_RX_NUM_BINS);

    // Force total_samples near its own wrap: bank 1 must NOT restart (no fresh
    // half-symbol offset), rows keep closing through the wrap.
    state.total_samples = 0xFFFFFFFFu;
    const uint32_t br_before = state.total_br;
    for (uint32_t i = 0; i < 4u * WSPR_SAMPLES_PER_SYMBOL; i++) {
        wspr_portapack_feed_sample(&state, 0.0f);
    }
    CHECK(state.bank1_started);
    CHECK_EQ(state.total_br, br_before + 8u);
}
#endif

TEST_CASE("absolute counter wrap: physical slots stay correct near 2^32") {
    wspr_portapack_state_t state;
    REQUIRE(wspr_portapack_init(&state));

    // Force the counter near its wrap. 2^32 is NOT a multiple of 352 (2^32 mod 352 =
    // 4096), so a naive (id + k) % 352 on raw ids would alias across the wrap; the glue
    // must keep writing slot = id % 352 through it.
    state.total_br = 0xFFFFFFFFu;
    state.total_samples = 0;

    // Feed two symbol periods: the counter wraps into ids 0.. and both banks' rows
    // keep landing in their own id's slot.
    Noise noise{4242};
    uint32_t completions = 0;
    for (uint32_t i = 0; i < 2u * WSPR_SAMPLES_PER_SYMBOL; i++) {
        if (wspr_portapack_feed_sample(&state, noise.gaussian())) completions++;
    }
    // Starting one id below 2^32, the first completion consumes id 0xFFFFFFFF and the
    // counter wraps to 0 on its way: total_br = completions - 1 after this phase.
    CHECK_EQ(state.total_br, completions - 1u);
    CHECK_EQ(completions, completions_for_rows(2u));

    // The rows must sit in the slots of their PRE-wrap ids: id 0xFFFFFFFF -> slot
    // 0xFFFFFFFF % 352, id 0 -> slot 0.
    const uint32_t slot_wrapped = phys_slot(0xFFFFFFFFu);
    CHECK_EQ(wspr_portapack_row_br(0), wspr_portapack_matrix());
    CHECK_EQ(wspr_portapack_row_br(0xFFFFFFFFu),
             (const uint8_t*)wspr_portapack_matrix() +
                 (size_t)slot_wrapped * WSPR_RX_NUM_BINS);
    CHECK_NE(slot_wrapped, 0u);
    CHECK_NE(slot_wrapped, 1u);
    // The rows carry real (nonzero) quantized bytes.
    CHECK_NE(wspr_portapack_row_br(0)[0], 0);
    CHECK_NE(wspr_portapack_row_br(0xFFFFFFFFu)[0], 0);

    // And a continuation keeps the wrapped ids sequential in their slots: every
    // completion after the wrap lands in slot = id % block_rows.
    for (uint32_t i = 0; i < 2u * WSPR_SAMPLES_PER_SYMBOL; i++) {
        wspr_portapack_feed_sample(&state, noise.gaussian());
    }
    // Steady state: OSR2 closes one block-row per half symbol, OSR1 one per symbol.
    CHECK_EQ(state.total_br, completions - 1u + (uint32_t)WSPR_RX_TIME_OSR * 2u);
    for (uint32_t id = 2u; id < state.total_br; id++) {
        CAPTURE(id);
        CHECK_EQ(wspr_portapack_row_br(id),
                 (const uint8_t*)wspr_portapack_matrix() +
                     (size_t)phys_slot(id) * WSPR_RX_NUM_BINS);
    }
}

TEST_CASE("a packet straddling the ring seam lands whole") {
    uint8_t tones[WSPR_CODED_BITS];
    REQUIRE(wspr_encode_message("K1ABC", "FN20", 37, tones));

    wspr_portapack_state_t state;
    REQUIRE(wspr_portapack_init(&state));

    // Pretend 300 block-rows already streamed (stale noise in every slot): the packet
    // that follows starts at absolute block-row 300, and its symbol rows 300 + 2n run
    // over the physical seam 351 -> 0.
    state.total_br = 300;

    std::vector<float> samples;
    synthesize(tones, 777, 0, (size_t)WSPR_RX_NUM_ROWS * WSPR_SAMPLES_PER_SYMBOL, samples);

    for (const float s : samples) wspr_portapack_feed_sample(&state, s);

    // Bank-0 symbol rows of the packet: absolute ids 300 + stride*n, physical slots
    // (300 + stride*n) % block_rows — under OSR2 the first 26 symbols sit past the
    // seam at the ring start.
    for (int n = 0; n < WSPR_CODED_BITS; n++) {
        const int col = 10 + tones[n];
        const uint32_t br = 300u + kSymStride * (uint32_t)n;
        const uint8_t* row = wspr_portapack_row_br(br);
        CAPTURE(n);
        CAPTURE(br);
        CAPTURE(phys_slot(br));
        CHECK(row_argmax(row) >= col - 1);
        CHECK(row_argmax(row) <= col + 1);
    }
}
