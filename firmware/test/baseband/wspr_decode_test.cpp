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

// Synthesize num_samples of stream audio: the transmission starting at
// absolute stream sample start_sample, tone n on bin base_bin + tones[n],
// plus unit-variance noise. snr_db is the WSJT-X display figure (noise
// referenced to 2500 Hz); the figure for the 6 Hz occupied bandwidth is
// 10*log10(2500/6) = 26.2 dB higher.
void synthesize_at(const uint8_t* tones,
                   int base_bin,
                   int start_sample,
                   int num_samples,
                   float snr_db,
                   uint32_t seed,
                   std::vector<float>& out) {
    const float snr6 = std::pow(10.0f, (snr_db + 26.2f) / 10.0f);
    // Unit-variance noise at 12000 sps carries 2*6/12000 of its power inside
    // the 6 Hz occupied band, the tone A^2/2: SNR6 = 500*A^2, so
    // A = sqrt(snr6/500).
    const float amp = std::sqrt(4.0f * snr6 * 6.0f / 12000.0f);
    Noise noise{seed};
    out.assign(num_samples, 0.0f);

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
        const size_t begin = (size_t)start_sample + (size_t)n * WSPR_SAMPLES_PER_SYMBOL;
        for (int i = 0; i < WSPR_SAMPLES_PER_SYMBOL && begin + i < out.size(); i++) {
            out[begin + i] += amp * std::sin(phase + f * phase_step * (float)i);
        }
        // Continuous phase across symbols.
        phase = std::fmod(phase + f * phase_step * (float)WSPR_SAMPLES_PER_SYMBOL, 6.2831853f);
    }
    for (size_t i = 0; i < out.size(); i++) {
        out[i] += noise.gaussian();
    }
}

// Synthesize kRows * 8192 samples: the transmission starting at
// start_row, tone n on bin base_bin + tones[n], plus unit-variance noise.
void synthesize(const uint8_t* tones,
                int base_bin,
                int start_row,
                float snr_db,
                uint32_t seed,
                std::vector<float>& out) {
    synthesize_at(tones, base_bin, start_row * WSPR_SAMPLES_PER_SYMBOL,
                  kRows * WSPR_SAMPLES_PER_SYMBOL, snr_db, seed, out);
}

// Reference Goertzel bank over num_rows consecutive 8192-sample rows of the
// stream beginning at start_sample: one power byte per (row, bin), the same
// quantization the baseband uses (mag_dB = byte * 0.5 - 120 on a scale where
// a full-scale tone lands high). A row whose span runs past the end of the
// stream is left zero, the way the receiver's window ends.
void goertzel_bank(const std::vector<float>& samples,
                   int start_sample,
                   int num_rows,
                   std::vector<uint8_t>& bank) {
    bank.assign((size_t)num_rows * kNumBins, 0);
    std::vector<double> coeff(kNumBins);
    for (int b = 0; b < kNumBins; b++) {
        const int k = kBin0 + b;
        coeff[b] = 2.0 * std::cos(6.2831853 * (double)k / (double)WSPR_SAMPLES_PER_SYMBOL);
    }
    std::vector<double> s1(kNumBins), s2(kNumBins);
    for (int row = 0; row < num_rows; row++) {
        const size_t begin = (size_t)start_sample + (size_t)row * WSPR_SAMPLES_PER_SYMBOL;
        if (begin + WSPR_SAMPLES_PER_SYMBOL > samples.size()) continue;
        std::fill(s1.begin(), s1.end(), 0.0);
        std::fill(s2.begin(), s2.end(), 0.0);
        for (int i = 0; i < WSPR_SAMPLES_PER_SYMBOL; i++) {
            const float x = samples[begin + i];
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
            bank[(size_t)row * kNumBins + b] = (uint8_t)byte;
        }
    }
}

// Single-bank matrix, the shape wspr_decode_window() consumes with
// time_osr = 1.
void goertzel_matrix(const std::vector<float>& samples, std::vector<uint8_t>& mag) {
    goertzel_bank(samples, 0, kRows, mag);
}

// Dual-bank matrix, the shape the receiver glue produces and
// wspr_decode_window() consumes with time_osr = 2: block-row index
// 2*row + bank, bank 0 rows closing at capture sample 0 (mod 8192), bank 1
// starting its first full row at capture sample 4096 and lagging half a
// symbol. Bank 1's last row runs past the capture end and stays zero, like
// the receiver's window.
void goertzel_matrix_dual(const std::vector<float>& samples,
                          int start_sample,
                          int num_rows,
                          std::vector<uint8_t>& mag) {
    std::vector<uint8_t> bank0, bank1;
    goertzel_bank(samples, start_sample, num_rows, bank0);
    goertzel_bank(samples, start_sample + WSPR_SAMPLES_PER_SYMBOL / 2, num_rows, bank1);
    mag.assign((size_t)2 * num_rows * kNumBins, 0);
    for (int row = 0; row < num_rows; row++) {
        std::memcpy(&mag[(size_t)(2 * row) * kNumBins], &bank0[(size_t)row * kNumBins], kNumBins);
        std::memcpy(&mag[(size_t)(2 * row + 1) * kNumBins],
                    &bank1[(size_t)row * kNumBins], kNumBins);
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
                              WSPR_MAX_RESULTS, 1);
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
    // The cluster table is file-static and doctest runs every case in one
    // process, so each case clears it before decoding.
    wspr_decode_reset_aggregation();
    const Capture cap = make_capture("K1ABC", "FN20", 37, kBin0 + 10, 1, 40.0f, 12345);
    wspr_decode_result_t results[WSPR_MAX_RESULTS];
    const int count = decode_capture(cap, results);
    REQUIRE(count >= 1);
    CHECK(contains(results, count, "K1ABC", "FN20", 37));
    // A single window can never claim aggregation: one slot per window per
    // cluster, and a cluster only decodes from two integrated windows on.
    for (int i = 0; i < count; i++) {
        CHECK_EQ(results[i].agg_windows, 0);
    }
    // The clean capture's tone bins sit far above the pair-bin noise mean;
    // the status line surfaces this as the live signal-quality readout.
    CHECK(results[0].sync_score > 20);
}

TEST_CASE("decodes at wspr-level noise") {
    // -20 dB and -26 dB on the WSJT scale; the spec target is -28 dB.
    wspr_decode_reset_aggregation();
    for (const float snr : {-20.0f, -26.0f}) {
        CAPTURE(snr);
        const Capture cap = make_capture("KV4XY", "EM10", 30, kBin0 + 20, 2, snr, 777);
        wspr_decode_result_t results[WSPR_MAX_RESULTS];
        const int count = decode_capture(cap, results);
        CHECK(contains(results, count, "KV4XY", "EM10", 30));
    }
}

TEST_CASE("decodes with time and frequency offsets") {
    wspr_decode_reset_aggregation();
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
            // The transmission starts at capture row 3; the slot tracker
            // steers this offset.
            CHECK_EQ(results[i].row_offset, 3);
            // At -24 dB the tone bins still rise above the pair-bin noise
            // mean, just barely: the status line readout stays positive.
            CHECK(results[i].sync_score > 0);
        }
    }
}

TEST_CASE("dual-bank decode survives an exact half-row misalignment") {
    // Each decode below runs against a cleared cluster table, so the single-
    // bank attempt cannot seed a cluster the dual-bank decode then completes
    // (the aggregated result would replace the per-window one this case
    // inspects).
    wspr_decode_reset_aggregation();
    // The v2 failure mode: the receiver's capture starts at an arbitrary
    // stream sample, so the transmission can start half a symbol away from
    // the row grid, and every row-quantized symbol then smears across two
    // rows. This capture starts at stream sample 12000 (1 s of dead air into
    // a slot) and the transmission starts 4096 samples later - exactly half a
    // row into the capture, the worst phase for bank 0 and the exact phase of
    // bank 1's grid.
    constexpr int kStartSample = 12000;
    constexpr int kTxOffset = WSPR_SAMPLES_PER_SYMBOL / 2;
    constexpr float kSnr = 0.0f;

    uint8_t tones[WSPR_CODED_BITS];
    REQUIRE(wspr_encode_message("K1ABC", "FN20", 37, tones));
    // The stream runs 1 s past the capture start, so the 176-row window fits
    // completely and only bank 1's last row (which always runs half a symbol
    // past the receiver's window end) is zero-filled, exactly like the glue.
    std::vector<float> samples;
    synthesize_at(tones, kBin0 + 20, kStartSample + kTxOffset,
                  kStartSample + kRows * WSPR_SAMPLES_PER_SYMBOL, kSnr, 60606, samples);

    // The dual-bank capture matrix; bank 0's rows are its even block-rows.
    std::vector<uint8_t> dual;
    goertzel_matrix_dual(samples, kStartSample, kRows, dual);
    std::vector<uint8_t> bank0((size_t)kRows * kNumBins);
    for (int row = 0; row < kRows; row++) {
        std::memcpy(&bank0[(size_t)row * kNumBins], &dual[(size_t)(2 * row) * kNumBins],
                    kNumBins);
    }

    // Single bank, the v1 behaviour: every symbol is smeared 50/50 across
    // two rows, and the decoder must NOT recover the message.
    wspr_decode_result_t results[WSPR_MAX_RESULTS];
    const int single_count = wspr_decode_window(bank0.data(), kRows, kNumBins,
                                                kBin0 * kBinStepNum / kBinStepDen,
                                                results, WSPR_MAX_RESULTS, 1);
    CHECK_FALSE(contains(results, single_count, "K1ABC", "FN20", 37));

    // Dual banks, the v2 behaviour: bank 1's row grid sits exactly on the
    // transmission's symbol boundaries, so the message must decode out of
    // bank 1 while the output stays a single merged result list. The cluster
    // table is cleared again first: the smeared single-bank attempt above may
    // have completed the payload below the gates, and its cluster must not
    // complete under this decode.
    wspr_decode_reset_aggregation();
    const int dual_count = wspr_decode_window(dual.data(), kRows, kNumBins,
                                              kBin0 * kBinStepNum / kBinStepDen,
                                              results, WSPR_MAX_RESULTS, 2);
    REQUIRE(dual_count >= 1);
    REQUIRE(contains(results, dual_count, "K1ABC", "FN20", 37));
    for (int i = 0; i < dual_count; i++) {
        if (strcmp(results[i].callsign, "K1ABC") == 0) {
            // The decode came off bank 1, whose rows are aligned: symbol row
            // 0 of that bank's grid, and the bank recorded per result.
            CHECK_EQ(results[i].bank, 1);
            CHECK_EQ(results[i].row_offset, 0);
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

TEST_CASE("noise-only window stays bounded") {
    // Without a CRC, roughly a seventh of random payloads unpack as a
    // structurally valid standard message, and the sync/metric gates cannot
    // fully separate that population from threshold-level signals (their
    // metrics overlap). The decoder therefore hands the app a bounded result
    // list and the report gate is the two-slot correlation the app applies
    // (real stations repeat within several windows, impostors are random).
    // This test pins the flood bound: a noise window must never yield more
    // results than the candidate budget allows after deduplication. The
    // windows run against ONE cluster table on purpose: their impostor
    // completions fill it, and the accumulation must not turn that fill into
    // aggregated decodes (see the accumulation case below).
    wspr_decode_reset_aggregation();
    for (const uint32_t seed : {21u, 4242u, 909090u}) {
        CAPTURE(seed);
        Noise noise{seed};
        std::vector<float> samples(kRows * WSPR_SAMPLES_PER_SYMBOL, 0.0f);
        for (auto& s : samples) s = noise.gaussian();
        std::vector<uint8_t> mag;
        goertzel_matrix(samples, mag);
        wspr_decode_result_t results[WSPR_MAX_RESULTS];
        const int count = wspr_decode_window(mag.data(), kRows, kNumBins,
                                             kBin0 * kBinStepNum / kBinStepDen,
                                             results, WSPR_MAX_RESULTS, 1);
        CHECK_LE(count, 3);
        for (int i = 0; i < count; i++) {
            CHECK_EQ(results[i].agg_windows, 0);
        }
    }
}

TEST_CASE("two slots integrate into a decode below the per-window threshold") {
    // The sensitivity extension: two captures of the same message at -26 dB
    // WSJT, different noise seeds, each below the per-window gates on its
    // own. Both windows complete the true payload at the true alignment with
    // agreement 97-99 but a path metric of 13-19, under the report gate of
    // 20, so neither window reports anything. The payload-keyed cluster
    // integrates the two slots' weights; the accumulated decode runs through
    // the same gates on the summed weights and clears them with margin.
    //
    // Both captures are synthesized with the same base bin and start row, so
    // their sym_weight arrays are transmission-aligned by construction (the
    // demod walks best_row + n, and symbol 0 is the transmission start in
    // both) - which is what lets the weights sum coherently in symbol index.
    wspr_decode_reset_aggregation();
    const Capture cap1 = make_capture("K1ABC", "FN20", 37, kBin0 + 10, 1, -26.0f, 11);
    const Capture cap2 = make_capture("K1ABC", "FN20", 37, kBin0 + 10, 1, -26.0f, 38);
    wspr_decode_result_t results[WSPR_MAX_RESULTS];

    const int count1 = decode_capture(cap1, results);
    INFO("slot 1 alone: ", count1, " results");
    CHECK_EQ(count1, 0);

    const int count2 = decode_capture(cap2, results);
    bool found = false;
    for (int i = 0; i < count2; i++) {
        if (strcmp(results[i].callsign, "K1ABC") == 0 && results[i].agg_windows >= 2) {
            found = true;
            // The aggregated result carries the integrated decode's quality:
            // agreement near 100, and the sum-scale path metric with margin
            // over the same gates the per-window decodes failed (agreement
            // floor 62, report metric floor 20).
            CHECK(results[i].agreement_pct >= 62);
            CHECK(results[i].metric >= 20);
            // Tone 0 of the cluster: the first contributing decode's audio
            // frequency, bin kBin0 + 10.
            CHECK_EQ(results[i].freq_hz,
                     kBin0 * kBinStepNum / kBinStepDen + 10 * kBinStepNum / kBinStepDen);
        }
    }
    REQUIRE(found);
    CHECK(contains(results, count2, "K1ABC", "FN20", 37));
}

TEST_CASE("repeated noise windows never aggregate") {
    // The adversarial accumulation case: pure noise fills the cluster table
    // with impostor completions (agreement-passing, unpack-valid, metric-gated
    // junk). Aggregated decodes additionally need the SAME payload twice -
    // independent noise never repeats one - so the table churns without ever
    // reporting. Six independent windows against one cluster table.
    wspr_decode_reset_aggregation();
    for (const uint32_t seed : {21u, 4242u, 909090u, 555u, 31337u, 7777u}) {
        CAPTURE(seed);
        Noise noise{seed};
        std::vector<float> samples(kRows * WSPR_SAMPLES_PER_SYMBOL, 0.0f);
        for (auto& s : samples) s = noise.gaussian();
        std::vector<uint8_t> mag;
        goertzel_matrix(samples, mag);
        wspr_decode_result_t results[WSPR_MAX_RESULTS];
        const int count = wspr_decode_window(mag.data(), kRows, kNumBins,
                                             kBin0 * kBinStepNum / kBinStepDen,
                                             results, WSPR_MAX_RESULTS, 1);
        CHECK_LE(count, 3);
        for (int i = 0; i < count; i++) {
            CHECK_EQ(results[i].agg_windows, 0);
        }
    }
}
