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
 * Host tests for the WSPR baseband glue. The glue turns 12 kHz audio into the
 * magnitude matrix wspr_decode_window() consumes, so these cases check that
 * contract end to end: a phase-continuous 4-FSK synthesis (the same generator
 * as wspr_decode_test.cpp uses, but loud enough that each symbol's tone is the
 * maximum of its row) must come back out of wspr_portapack_feed_sample with
 * every tone in the column the protocol puts it in.
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
// the tone. (A quieter tone keeping the byte gap at 6-10 would leave the row maximum to
// chance: the noise bytes scatter with a sigma of ~11.)
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

// Synthesize WSPR_RX_NUM_ROWS rows of 12 kHz audio: the transmission starting at row 0,
// tone n on absolute bin kSynthBaseBin + tones[n] with continuous phase across symbol
// boundaries, plus unit-variance white noise.
void synthesize(const uint8_t* tones, uint32_t seed, std::vector<float>& out) {
    Noise noise{seed};
    out.assign((size_t)WSPR_RX_NUM_ROWS * WSPR_SAMPLES_PER_SYMBOL, 0.0f);

    const float two_pi = 6.2831853f;
    float phase = 0.0f;
    for (int n = 0; n < WSPR_CODED_BITS; n++) {
        const int bin = kSynthBaseBin + tones[n];
        // Per-sample angular increment of a tone sitting on absolute bin `bin`:
        // 2*pi*bin/8192 rad, i.e. 2*pi*f/f_sample with f = bin*12000/8192 Hz.
        const float omega = two_pi * (float)bin / (float)WSPR_SAMPLES_PER_SYMBOL;
        const size_t begin = (size_t)n * WSPR_SAMPLES_PER_SYMBOL;
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
int row_argmax(const uint8_t* mag, int row) {
    const uint8_t* r = mag + (size_t)row * WSPR_RX_NUM_BINS;
    int best_col = 0;
    for (int b = 1; b < WSPR_RX_NUM_BINS; b++) {
        if (r[b] > r[best_col]) best_col = b;
    }
    return best_col;
}

bool row_has_nonzero(const uint8_t* mag, int row) {
    const uint8_t* r = mag + (size_t)row * WSPR_RX_NUM_BINS;
    for (int b = 0; b < WSPR_RX_NUM_BINS; b++) {
        if (r[b] != 0) return true;
    }
    return false;
}

}  // namespace

TEST_CASE("init and free set up and clear the glue") {
    // Null guards: no crash, no window.
    CHECK_FALSE(wspr_portapack_init(nullptr));
    CHECK_FALSE(wspr_portapack_feed_sample(nullptr, 0.0f));
    wspr_portapack_free(nullptr);
    wspr_portapack_reset_window(nullptr);

    wspr_portapack_state_t state;
    REQUIRE(wspr_portapack_init(&state));
    CHECK_EQ(state.rows_filled, 0);
    CHECK_EQ(state.samples_in_row, 0);
    CHECK_FALSE(state.window_full);

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
    CHECK_EQ(state.samples_in_row, 3);

    // Nothing is heap-allocated; free just clears the state.
    wspr_portapack_free(&state);
    CHECK_EQ(state.rows_filled, 0);
    CHECK_EQ(state.samples_in_row, 0);
    CHECK_FALSE(state.window_full);
}

TEST_CASE("a full window lands every tone in its column") {
    uint8_t tones[WSPR_CODED_BITS];
    REQUIRE(wspr_encode_message("K1ABC", "FN20", 37, tones));

    std::vector<float> samples;
    synthesize(tones, 12345, samples);

    wspr_portapack_state_t state;
    REQUIRE(wspr_portapack_init(&state));

    const size_t total = (size_t)WSPR_RX_NUM_ROWS * WSPR_SAMPLES_PER_SYMBOL;
    bool full = false;
    size_t first_full = 0;
    for (size_t i = 0; i < total; i++) {
        full = wspr_portapack_feed_sample(&state, samples[i]);
        if (full && first_full == 0) first_full = i + 1;

        // Row and sample counters track the feed exactly.
        if (i + 1 == WSPR_SAMPLES_PER_SYMBOL) {
            CHECK_EQ(state.rows_filled, 1);
            CHECK_EQ(state.samples_in_row, 0);
        }
        if (i + 1 == 90 * WSPR_SAMPLES_PER_SYMBOL) {
            CHECK_EQ(state.rows_filled, 90);
        }
        if (i + 1 == 90 * WSPR_SAMPLES_PER_SYMBOL + 100) {
            CHECK_EQ(state.rows_filled, 90);
            CHECK_EQ(state.samples_in_row, 100);
            CHECK_FALSE(full);
        }
    }

    // The window fills exactly on the last sample of row 175, not earlier.
    CHECK_EQ(first_full, total);
    CHECK(full);
    CHECK(state.window_full);
    CHECK_EQ(state.rows_filled, WSPR_RX_NUM_ROWS);
    CHECK_EQ(state.samples_in_row, 0);

    const uint8_t* mag = wspr_portapack_matrix();

    // Start, middle and end of the transmission: in bank 0 the tone sits in its
    // own column of its symbol's block-row (2*sym, bank 0 = even block rows) at
    // the byte the scale promises. Bank 1's rows straddle two symbols, so its
    // argmax is checked in the dedicated case below.
    for (const int sym : {0, 80, 161}) {
        const int col = 10 + tones[sym];
        const size_t block = (size_t)2 * sym;
        const int byte = mag[block * WSPR_RX_NUM_BINS + col];
        CAPTURE(sym);
        CAPTURE(tones[sym]);
        CAPTURE(col);
        CAPTURE(byte);
        CHECK(byte >= 185);
        CHECK(byte <= 205);
        CHECK(row_argmax(mag, (int)block) >= col - 1);
        CHECK(row_argmax(mag, (int)block) <= col + 1);
    }

    // And every bank-0 signal row: the tone must be the row maximum, so the
    // decoder's neighbour comparisons find the tones where the protocol put
    // them. Bank 1's rows straddle two symbols (they start half a row into the
    // capture), so each straddling row's maximum sits in one of the two
    // adjacent tone columns.
    for (int n = 0; n < WSPR_CODED_BITS; n++) {
        const int col = 10 + tones[n];
        CAPTURE(n);
        CAPTURE(tones[n]);
        CAPTURE(col);
        CHECK(row_argmax(mag, 2 * n) >= col - 1);
        CHECK(row_argmax(mag, 2 * n) <= col + 1);
    }
    for (int k = 0; k + 1 < WSPR_CODED_BITS; k++) {
        const int col_a = 10 + tones[k];
        const int col_b = 10 + tones[k + 1];
        const int am = row_argmax(mag, 2 * k + 1);
        CAPTURE(k);
        CAPTURE(am);
        // The straddling row holds half of symbol k and half of symbol k+1;
        // whichever half-tone is louder wins the argmax, so either column is
        // a pass (within the +/-1 alignment slack).
        const bool near_a = am >= col_a - 1 && am <= col_a + 1;
        const bool near_b = am >= col_b - 1 && am <= col_b + 1;
        const bool straddle_ok = near_a || near_b;
        CHECK(straddle_ok);
    }

    // Rows past the transmission carry noise alone: well below the tone bytes and far
    // from the 255 pin. Bank 0's rows past symbol 161 are the even block rows from
    // 2*166 on; bank 1's rows straddle, so only even block rows are checked here.
    for (int sym = 166; sym < WSPR_RX_NUM_ROWS; sym++) {
        const int row = 2 * sym;
        CAPTURE(row);
        CHECK(mag[(size_t)row * WSPR_RX_NUM_BINS + row_argmax(mag, row)] < 190);
    }

    // Once full, feeding keeps returning true and touches nothing.
    const size_t tone_cell = (size_t)80 * WSPR_RX_NUM_BINS + 10 + tones[80];
    const uint8_t byte_before = mag[tone_cell];
    CHECK(wspr_portapack_feed_sample(&state, 0.0f));
    CHECK_EQ(state.rows_filled, WSPR_RX_NUM_ROWS);
    CHECK_EQ(state.samples_in_row, 0);
    CHECK_EQ(mag[tone_cell], byte_before);
}

TEST_CASE("reset_window clears the matrix and the counters") {
    wspr_portapack_state_t state;
    REQUIRE(wspr_portapack_init(&state));

    // Noise alone is enough to fill rows with real (nonzero) bytes.
    Noise noise{999};
    const int fed = 2 * WSPR_SAMPLES_PER_SYMBOL + 100;
    for (int i = 0; i < fed; i++) {
        REQUIRE_FALSE(wspr_portapack_feed_sample(&state, noise.gaussian()));
    }
    CHECK_EQ(state.rows_filled, 2);
    CHECK_EQ(state.samples_in_row, 100);
    CHECK(row_has_nonzero(wspr_portapack_matrix(), 0));
    CHECK(row_has_nonzero(wspr_portapack_matrix(), 1));

    wspr_portapack_reset_window(&state);

    const std::vector<uint8_t> zeros(WSPR_RX_MATRIX_SIZE, 0);
    CHECK_EQ(std::memcmp(wspr_portapack_matrix(), zeros.data(), WSPR_RX_MATRIX_SIZE), 0);
    CHECK_EQ(state.rows_filled, 0);
    CHECK_EQ(state.samples_in_row, 0);
    CHECK_FALSE(state.window_full);

    // Capture resumes cleanly from the empty window.
    for (int i = 0; i < 5; i++) {
        CHECK_FALSE(wspr_portapack_feed_sample(&state, noise.gaussian()));
    }
    CHECK_EQ(state.rows_filled, 0);
    CHECK_EQ(state.samples_in_row, 5);

    // Re-init also has to clear: it is what a restart of the app runs through.
    for (int i = 0; i < WSPR_SAMPLES_PER_SYMBOL; i++) {
        CHECK_FALSE(wspr_portapack_feed_sample(&state, noise.gaussian()));
    }
    REQUIRE(wspr_portapack_init(&state));
    CHECK_EQ(std::memcmp(wspr_portapack_matrix(), zeros.data(), WSPR_RX_MATRIX_SIZE), 0);
    CHECK_EQ(state.rows_filled, 0);
    CHECK_EQ(state.samples_in_row, 0);
    CHECK_FALSE(state.window_full);
}
