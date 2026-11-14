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
 * Host tests for the WSPR decoder. Each case synthesizes a 120 s capture in
 * the time domain (phase-continuous 4-FSK at exact bin frequencies plus
 * white noise at a controlled SNR), runs a reference Goertzel bank over it
 * with the byte quantization the baseband receiver uses, and decodes the
 * result. SNR parameters are WSJT-X display-scale figures (2500 Hz noise
 * reference); the synthesize() helper converts them to the 6 Hz
 * occupied-bandwidth figure the amplitude math needs (26.2 dB higher), so
 * the -26 dB case below is the WSJT-X weak-signal regime, not 6 Hz -26 dB.
 */

#include "wspr_decode.h"
#include "wspr_encode.h"
#include "wspr_unpack.h"

#include <doctest.h>

#include <cmath>
#include <cstring>
#include <vector>

namespace {

constexpr int kRows = 176;
constexpr int kNumBins = 137;  // 1400-1600 Hz in 1.4648 Hz steps
constexpr int kBin0 = 956;     // 956 * 12000/8192 = 1400.0 Hz
constexpr int kBinStepNum = 12000;
constexpr int kBinStepDen = 8192;

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

// Synthesize num_rows * 8192 samples: the transmission starting at
// start_row, tone n on bin base_bin + tones[n], plus unit-variance noise.
// snr_db is the WSJT-X display figure (noise referenced to 2500 Hz); the
// figure for the 6 Hz occupied bandwidth is 10*log10(2500/6) = 26.2 dB
// higher.
void synthesize(const uint8_t* tones,
                int base_bin,
                int start_row,
                float snr_db,
                uint32_t seed,
                std::vector<float>& out) {
    const float snr6 = std::pow(10.0f, (snr_db + 26.2f) / 10.0f);
    // Unit-variance noise at 12000 sps carries 2*6/12000 of its power inside
    // the 6 Hz occupied band, the tone A^2/2: SNR6 = 500*A^2, so
    // A = sqrt(snr6/500).
    const float amp = std::sqrt(4.0f * snr6 * 6.0f / 12000.0f);
    Noise noise{seed};
    out.assign(kRows * WSPR_SAMPLES_PER_SYMBOL, 0.0f);

    // Tone of f Hz at 12000 sps: the phase advances 2*pi*f/12000 per sample.
    // A bin-aligned tone completes an integer number of cycles (bin cycles)
    // per 8192-sample symbol, so the accumulator below returns to its start
    // value at every symbol boundary and the synthesis is phase-continuous
    // by construction.
    const float phase_step = 6.2831853f / (float)kBinStepNum;
    float phase = 0.0f;
    for (int n = 0; n < WSPR_CODED_BITS; n++) {
        const int bin = base_bin + tones[n];
        const float f = (float)(bin * kBinStepNum) / (float)kBinStepDen;
        const int begin = start_row * WSPR_SAMPLES_PER_SYMBOL + n * WSPR_SAMPLES_PER_SYMBOL;
        for (int i = 0; i < WSPR_SAMPLES_PER_SYMBOL; i++) {
            out[begin + i] += amp * std::sin(phase + f * phase_step * (float)i);
        }
        // Continuous phase across symbols.
        phase = std::fmod(phase + f * phase_step * (float)WSPR_SAMPLES_PER_SYMBOL, 6.2831853f);
    }
    for (size_t i = 0; i < out.size(); i++) {
        out[i] += noise.gaussian();
    }
}

// Reference Goertzel bank: one power byte per (row, bin), the same
// quantization the baseband uses (mag_dB = byte * 0.5 - 120 on a scale where
// a full-scale tone lands high).
void goertzel_matrix(const std::vector<float>& samples, std::vector<uint8_t>& mag) {
    mag.assign(kRows * kNumBins, 0);
    std::vector<double> coeff(kNumBins);
    for (int b = 0; b < kNumBins; b++) {
        const int k = kBin0 + b;
        coeff[b] = 2.0 * std::cos(6.2831853 * (double)k / (double)WSPR_SAMPLES_PER_SYMBOL);
    }
    std::vector<double> s1(kNumBins), s2(kNumBins);
    for (int row = 0; row < kRows; row++) {
        std::fill(s1.begin(), s1.end(), 0.0);
        std::fill(s2.begin(), s2.end(), 0.0);
        for (int i = 0; i < WSPR_SAMPLES_PER_SYMBOL; i++) {
            const float x = samples[row * WSPR_SAMPLES_PER_SYMBOL + i];
            for (int b = 0; b < kNumBins; b++) {
                const double s0 = (double)x + coeff[b] * s1[b] - s2[b];
                s2[b] = s1[b];
                s1[b] = s0;
            }
        }
        for (int b = 0; b < kNumBins; b++) {
            const double power =
                (s1[b] * s1[b] + s2[b] * s2[b] - coeff[b] * s1[b] * s2[b]) /
                ((double)WSPR_SAMPLES_PER_SYMBOL * (double)WSPR_SAMPLES_PER_SYMBOL);
            if (power < 1e-20) continue;
            int byte = (int)(6.0206 * std::log2(power) + 240.0);
            if (byte < 0) byte = 0;
            if (byte > 255) byte = 255;
            mag[row * kNumBins + b] = (uint8_t)byte;
        }
    }
}

struct Capture {
    std::vector<uint8_t> tones;
    std::vector<uint8_t> mag;
    int base_bin;
    int start_row;
};

Capture make_capture(const char* call, const char* grid, int dbm, int base_bin, int start_row, float snr_db_wsjt, uint32_t seed) {
    Capture cap;
    cap.tones.resize(WSPR_CODED_BITS);
    REQUIRE(wspr_encode_message(call, grid, dbm, cap.tones.data()));
    cap.base_bin = base_bin;
    cap.start_row = start_row;
    std::vector<float> samples;
    synthesize(cap.tones.data(), base_bin, start_row, snr_db_wsjt, seed, samples);
    goertzel_matrix(samples, cap.mag);
    return cap;
}

int decode_capture(const Capture& cap, wspr_decode_result_t* results) {
    return wspr_decode_window(cap.mag.data(), kRows, kNumBins,
                              kBin0 * kBinStepNum / kBinStepDen, results,
                              WSPR_MAX_RESULTS);
}

bool contains(const wspr_decode_result_t* results, int count, const char* call, const char* grid, int dbm) {
    for (int i = 0; i < count; i++) {
        if (strcmp(results[i].callsign, call) == 0 &&
            strcmp(results[i].grid, grid) == 0 &&
            results[i].dbm == dbm) {
            return true;
        }
    }
    return false;
}

}  // namespace

TEST_CASE("clean signal decodes at exact alignment") {
    const Capture cap = make_capture("K1ABC", "FN20", 37, kBin0 + 10, 1, 40.0f, 12345);
    wspr_decode_result_t results[WSPR_MAX_RESULTS];
    const int count = decode_capture(cap, results);
    REQUIRE(count >= 1);
    CHECK(contains(results, count, "K1ABC", "FN20", 37));
}

TEST_CASE("decodes at wspr-level noise") {
    // -20 dB and -26 dB on the WSJT scale; the spec target is -28 dB.
    for (const float snr : {-20.0f, -26.0f}) {
        CAPTURE(snr);
        const Capture cap = make_capture("KV4XY", "EM10", 30, kBin0 + 20, 2, snr, 777);
        wspr_decode_result_t results[WSPR_MAX_RESULTS];
        const int count = decode_capture(cap, results);
        CHECK(contains(results, count, "KV4XY", "EM10", 30));
    }
}

TEST_CASE("decodes with time and frequency offsets") {
    const Capture cap = make_capture("WX2YZ", "JJ19", 27, kBin0 + 45, 3, -24.0f, 4242);
    wspr_decode_result_t results[WSPR_MAX_RESULTS];
    const int count = decode_capture(cap, results);
    REQUIRE(count >= 1);
    CHECK(contains(results, count, "WX2YZ", "JJ19", 27));
    for (int i = 0; i < count; i++) {
        if (strcmp(results[i].callsign, "WX2YZ") == 0) {
            // Tone 0 sits on bin kBin0 + 45 = 1465.9 Hz.
            CHECK(results[i].freq_hz > 1464);
            CHECK(results[i].freq_hz < 1468);
        }
    }
}

TEST_CASE("unpack inverts packing for representative messages") {
    const char* cases[][2] = {
        {"K1ABC", "FN20"},
        {"AB1D", "JJ19"},
        {"XX9XYZ", "RR99"},
        {"M6KWH", "IO92"},
    };
    for (const auto& c : cases) {
        uint8_t bits[WSPR_SOURCE_BYTES];
        REQUIRE(wspr_pack_message(c[0], c[1], 33, bits));
        wspr_message_t message;
        REQUIRE(wspr_unpack_message(bits, &message));
        CHECK_EQ(std::strcmp(message.callsign, c[0]), 0);
        CHECK_EQ(std::strcmp(message.grid, c[1]), 0);
        CHECK_EQ(message.dbm, 33);
    }
    // Non-standard payloads must be refused: type marker cleared.
    uint8_t bits[WSPR_SOURCE_BYTES] = {0};
    wspr_message_t message;
    CHECK_FALSE(wspr_unpack_message(bits, &message));
}
