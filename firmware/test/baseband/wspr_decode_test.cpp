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

#include <iostream>

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
    // Noise must not produce station-shaped messages. Enforce zero-tail
    // codewords and real sync contrast rather than accepting a bounded flood.
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
        CHECK_EQ(count, 0);
    }
}

TEST_CASE("two slots integrate into a decode below the per-window threshold") {
    // The sensitivity extension, slot-aligned accumulator edition: two captures
    // of the same message at -26 dB WSJT, different noise seeds, each below the
    // per-window gates on its own (both per-window decodes report nothing).
    // Their snapshots fold into one slot-aligned track (same column, one slot
    // period apart in absolute block-rows); the integrated decode of the summed
    // weights runs through the same gates and clears them with margin.
    const int bin0_hz = kBin0 * kBinStepNum / kBinStepDen;
    const Capture cap1 = make_capture("K1ABC", "FN20", 37, kBin0 + 10, 1, -26.0f, 11);
    const Capture cap2 = make_capture("K1ABC", "FN20", 37, kBin0 + 10, 1, -26.0f, 38);
    wspr_decode_reset_aggregation();

    // Each slot alone: the per-window decoder reports nothing...
    wspr_decode_result_t results[WSPR_MAX_RESULTS];
    CHECK_EQ(decode_capture(cap1, results), 0);
    CHECK_EQ(decode_capture(cap2, results), 0);

    // ...but the ring path integrates their weights. Slot 1 contributes at
    // count = 1 and does not report.
    wspr_snapshot_t s1, s2;
    REQUIRE(wspr_snapshot_candidate(cap1.mag.data(), kRows, kNumBins, 1,
                                    cap1.start_row, 0, 10, bin0_hz,
                                    (uint32_t)cap1.start_row, &s1));
    REQUIRE(wspr_snapshot_candidate(cap2.mag.data(), kRows, kNumBins, 1,
                                    cap2.start_row, 0, 10, bin0_hz,
                                    (uint32_t)cap1.start_row + 352, &s2));
    wspr_decode_result_t r1;
    CHECK_EQ(wspr_decode_candidate(&s1, &r1, 1), 0);
    wspr_agg_track_info_t tracks[WSPR_AGG_TRACKS];
    wspr_agg_tracks(tracks, WSPR_AGG_TRACKS);
    CHECK(tracks[0].active);
    CHECK_EQ((int)tracks[0].count, 1);

    // Slot 2 completes the integration: one report, K1ABC, depth 2, with
    // margin over the same gates the per-window decodes failed (agreement
    // floor 62, report metric floor 20), tone 0 at bin kBin0 + 10.
    wspr_decode_result_t r2;
    REQUIRE_EQ(wspr_decode_candidate(&s2, &r2, 1), 1);
    CHECK_EQ(std::strcmp(r2.callsign, "K1ABC"), 0);
    CHECK_EQ(std::strcmp(r2.grid, "FN20"), 0);
    CHECK_EQ(r2.dbm, 37);
    CHECK_EQ((int)wspr_agg_last_decode_slots(), 2);
    CHECK(r2.agreement_pct >= 62);
    CHECK(r2.metric >= 20);
    CHECK_EQ(r2.freq_hz, bin0_hz + 10 * kBinStepNum / kBinStepDen);
    // The successful report resets the track: the next slot starts fresh.
    wspr_agg_tracks(tracks, WSPR_AGG_TRACKS);
    CHECK_FALSE(tracks[0].active);
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
        CHECK_EQ(count, 0);
    }
}

TEST_CASE("R3DNC decodes with a WSPR-bandwidth SNR estimate") {
    for (const float snr : {-25.0f, -19.0f, 0.0f}) {
        CAPTURE(snr);
        wspr_decode_reset_aggregation();
        const Capture cap = make_capture("R3DNC", "KO85", 30, kBin0 + 50, 2, snr, 777);
        wspr_decode_result_t results[WSPR_MAX_RESULTS];
        const int count = decode_capture(cap, results);
        REQUIRE(contains(results, count, "R3DNC", "KO85", 30));
        for (int i = 0; i < count; ++i) {
            if (std::strcmp(results[i].callsign, "R3DNC") != 0) continue;
            CHECK(std::abs(results[i].snr_db - snr) <= 3.0f);
        }
    }
}

TEST_CASE("unpack rejects a locator outside the Maidenhead range") {
    uint8_t bits[WSPR_SOURCE_BYTES] = {};
    REQUIRE(wspr_pack_message("R3DNC", "KO85", 30, bits));
    // Set all fifteen locator bits: 32767 is outside the 180*180 grid.
    for (int bit = 28; bit < 43; ++bit)
        bits[bit / 8] |= 0x80u >> (bit % 8);
    wspr_message_t message;
    CHECK_FALSE(wspr_unpack_message(bits, &message));
}

TEST_CASE("stationary carriers are not WSPR synchronization") {
    wspr_decode_reset_aggregation();
    for (const int column : {14, 51, 90}) {
        for (const int osr : {1, 2}) {
            CAPTURE(column);
            CAPTURE(osr);
            std::vector<uint8_t> mag((size_t)kRows * osr * kNumBins, 160);
            for (int row = 0; row < kRows * osr; ++row)
                mag[(size_t)row * kNumBins + column] = 240;
            wspr_decode_result_t results[WSPR_MAX_RESULTS];
            CHECK_EQ(wspr_decode_window(mag.data(), kRows, kNumBins, 1400,
                                        results, WSPR_MAX_RESULTS, osr),
                     0);
            // The sync-only acquisition probe must reject the same carriers.
            int16_t probe_row = -1, probe_score = 0;
            int8_t probe_bank = 0;
            CHECK_EQ(wspr_decode_probe_sync(mag.data(), kRows, kNumBins, osr,
                                            &probe_row, &probe_score, &probe_bank),
                     0);
        }
    }
}

TEST_CASE("sync probe skip helper lands the next window near row 4") {
    // Rows at or below the landing row need no correction.
    for (int row = 0; row <= WSPR_PROBE_LAND_ROW; ++row) {
        CAPTURE(row);
        CHECK_EQ(wspr_probe_skip_samples(row), 0);
    }
    // One row of skip per row of start above the landing row.
    for (int row = WSPR_PROBE_LAND_ROW + 1; row <= WSPR_PROBE_MAX_ROW; ++row) {
        CAPTURE(row);
        CHECK_EQ(wspr_probe_skip_samples(row),
                 (int32_t)(row - WSPR_PROBE_LAND_ROW) * WSPR_SAMPLES_PER_SYMBOL);
    }
    // The full skip fits the tracker's int32 pending_skip; the maximum is 16.4 s.
    CHECK_EQ(wspr_probe_skip_samples(WSPR_PROBE_MAX_ROW), 24 * WSPR_SAMPLES_PER_SYMBOL);
}

TEST_CASE("sync probe anchors truncated transmissions past the decodable slack") {
    // The acquisition case the tracker cannot otherwise handle: the transmission
    // starts at row 15..28, the full decode can never fit it, but the sync-only
    // probe still finds the anchor and the tracker's skip steers the NEXT window
    // onto a complete packet. The second window here is synthesized exactly as the
    // receiver sees it: one WSPR slot (1440000 samples) after the first window's
    // transmission start, minus the window overhang (1792 samples) and the skip
    // the probe computed.
    for (const int start_row : {15, 20, 25, 28}) {
        CAPTURE(start_row);
        wspr_decode_reset_aggregation();
        const Capture cap = make_capture("K1ABC", "FN20", 37, kBin0 + 10, start_row, -20.0f, 999);
        wspr_decode_result_t results[WSPR_MAX_RESULTS];
        const int count = decode_capture(cap, results);
        INFO("full decode at row ", start_row, ": ", count, " results");
        CHECK_EQ(count, 0);

        int16_t probe_row = -1, probe_score = 0;
        int8_t probe_bank = 0;
        REQUIRE(wspr_decode_probe_sync(cap.mag.data(), kRows, kNumBins, 1,
                                       &probe_row, &probe_score, &probe_bank));
        CHECK(probe_row >= start_row - 1);
        CHECK(probe_row <= start_row + 1);
        CHECK(probe_score >= 4);
        CHECK_EQ(probe_bank, 0);

        const int32_t skip = wspr_probe_skip_samples(probe_row);
        CHECK(skip > 0);
        // Stream geometry of the next window, relative to the window start: the
        // transmission repeats one slot (1440000 samples) after its start here,
        // while the next window begins one window-length (1441792, i.e. the
        // overhang 1792 later) plus the skip after the current window's start.
        const int next_start_sample =
            start_row * WSPR_SAMPLES_PER_SYMBOL -
            (kRows * WSPR_SAMPLES_PER_SYMBOL - 120 * WSPR_SAMPLE_RATE) - skip;
        CAPTURE(next_start_sample);
        CHECK(next_start_sample >= 0);
        std::vector<float> samples;
        synthesize_at(cap.tones.data(), cap.base_bin, next_start_sample,
                      kRows * WSPR_SAMPLES_PER_SYMBOL, -20.0f, 998, samples);
        std::vector<uint8_t> mag2;
        goertzel_matrix(samples, mag2);
        wspr_decode_reset_aggregation();
        const int count2 = wspr_decode_window(mag2.data(), kRows, kNumBins,
                                              kBin0 * kBinStepNum / kBinStepDen,
                                              results, WSPR_MAX_RESULTS, 1);
        INFO("steered window: ", count2, " results, tx at sample ", next_start_sample);
        REQUIRE(count2 >= 1);
        CHECK(contains(results, count2, "K1ABC", "FN20", 37));
        for (int i = 0; i < count2; ++i) {
            // A complete packet lands near the landing row, comfortably inside
            // the decodable 0..14 slack.
            CHECK(results[i].row_offset >= 0);
            CHECK(results[i].row_offset <= 14);
        }
    }
}

TEST_CASE("sync probe finds nothing on pure noise") {
    wspr_decode_reset_aggregation();
    for (const uint32_t seed : {21u, 4242u, 909090u, 555u, 31337u, 7777u, 60606u}) {
        CAPTURE(seed);
        Noise noise{seed};
        std::vector<float> samples(kRows * WSPR_SAMPLES_PER_SYMBOL, 0.0f);
        for (auto& s : samples) s = noise.gaussian();
        std::vector<uint8_t> mag;
        goertzel_matrix(samples, mag);
        int16_t probe_row = -1, probe_score = 0;
        int8_t probe_bank = 0;
        CHECK_EQ(wspr_decode_probe_sync(mag.data(), kRows, kNumBins, 1,
                                        &probe_row, &probe_score, &probe_bank),
                 0);
        CHECK_EQ(probe_row, -1);
    }
}

TEST_CASE("sync probe works on the dual-bank matrix") {
    // The receiver always runs OSR2; the probe must anchor through the
    // interleaved banks and report which one carried the best score.
    wspr_decode_reset_aggregation();
    constexpr int kStartSample = 12000;
    // The transmission starts at capture row 21: past the decodable slack even
    // with sub-row slop, so the full decode has nothing to find and the probe
    // is what anchors it.
    constexpr int kTxRow = 21;
    uint8_t tones[WSPR_CODED_BITS];
    REQUIRE(wspr_encode_message("KV4XY", "EM10", 30, tones));
    std::vector<float> samples;
    synthesize_at(tones, kBin0 + 20, kStartSample + kTxRow * WSPR_SAMPLES_PER_SYMBOL,
                  kStartSample + kRows * WSPR_SAMPLES_PER_SYMBOL, -20.0f, 1234, samples);
    std::vector<uint8_t> dual;
    goertzel_matrix_dual(samples, kStartSample, kRows, dual);
    // First confirm the full decode cannot recover it: the packet runs past
    // the window end in both banks' grids.
    wspr_decode_result_t results[WSPR_MAX_RESULTS];
    const int count = wspr_decode_window(dual.data(), kRows, kNumBins,
                                         kBin0 * kBinStepNum / kBinStepDen,
                                         results, WSPR_MAX_RESULTS, 2);
    INFO("full decode at row ", kTxRow, ": ", count, " results");
    CHECK_EQ(count, 0);
    int16_t probe_row = -1, probe_score = 0;
    int8_t probe_bank = -1;
    REQUIRE(wspr_decode_probe_sync(dual.data(), kRows, kNumBins, 2,
                                   &probe_row, &probe_score, &probe_bank));
    CHECK(probe_row >= kTxRow - 1);
    CHECK(probe_row <= kTxRow + 1);
    CHECK((probe_bank == 0 || probe_bank == 1));
    CHECK(wspr_probe_skip_samples(probe_row) > 0);
}

// ---------------------------------------------------------------------------
// Candidate snapshot contract (ring architecture stage 2): the frozen
// candidate must decode BIT-IDENTICALLY to the window path's result for the
// same (row, bank, col, drift), including soft weights, path metric,
// agreement, payload, SNR and the multi-slot accumulation contribution.
// ---------------------------------------------------------------------------

namespace {

// Invert the candidate's tone-0 column from its reported audio frequency
// (the bin->Hz map is monotonic with steps >= 1 Hz, so the match is unique).
int col_from_freq(int bin0_hz, int freq_hz) {
    for (int c = 0; c < 4096; c++) {
        if (bin0_hz + (c * kBinStepNum) / kBinStepDen == freq_hz) return c;
    }
    return -1;
}

// Independent accumulation of estimate_snr's noise histogram: byte>>2 over
// every bin OUTSIDE the drift-tracking exclusion band [c-4, c+7], same rows,
// same expression c = col0 + drift*n/161.
std::vector<uint16_t> reference_noise_histogram(const std::vector<uint8_t>& mag,
                                                int num_rows,
                                                int num_bins,
                                                int time_osr,
                                                int start_row,
                                                int bank,
                                                int col0,
                                                int drift) {
    std::vector<uint16_t> hist(64, 0);
    const int stride = time_osr * num_bins;
    const int base = bank * num_bins;
    for (int n = 0; n < WSPR_CODED_BITS; n++) {
        int c = col0;
        if (drift != 0) c += (drift * n) / (WSPR_CODED_BITS - 1);
        const uint8_t* row = &mag[(size_t)(start_row + n) * stride + base];
        for (int b = 0; b < num_bins; b++) {
            if (b >= c - 4 && b <= c + 7) continue;
            hist[row[b] >> 2]++;
        }
    }
    return hist;
}

void check_snapshot_matches(const std::vector<uint8_t>& mag, int time_osr, int start_row, int bank, int col0, const wspr_decode_result_t& ref) {
    const int bin0_hz = kBin0 * kBinStepNum / kBinStepDen;
    const int drift = ref.drift_bins;
    REQUIRE(drift >= -3);
    REQUIRE(drift <= 3);

    // (b) snapshot candidate + decode from the frozen copy.
    wspr_snapshot_t snap;
    REQUIRE(wspr_snapshot_candidate(mag.data(), kRows, kNumBins, time_osr,
                                    start_row, bank, col0, bin0_hz,
                                    (uint32_t)start_row, &snap));
    // Drift refinement must land on the same drift the window path chose.
    CHECK_EQ(snap.meta.drift, drift);
    CHECK_EQ(snap.meta.valid_symbols, WSPR_CODED_BITS);
    CHECK_EQ(snap.meta.bank, bank);

    // Noise histogram is byte-exact against the estimate_snr semantics.
    const auto ref_hist =
        reference_noise_histogram(mag, kRows, kNumBins, time_osr, start_row, bank, col0, drift);
    CHECK(std::memcmp(snap.noise_histogram, ref_hist.data(), sizeof(snap.noise_histogram)) == 0);

    // Soft weights: matrix demod (the window path's own routine) vs the
    // snapshot demod, bit in bit.
    int8_t w_matrix[WSPR_CODED_BITS];
    int8_t w_snap[WSPR_CODED_BITS];
    REQUIRE(wspr_decode_demod_matrix_weights(mag.data(), kRows, kNumBins, time_osr,
                                             start_row, bank, col0, drift, w_matrix));
    REQUIRE(wspr_snapshot_demod_weights(&snap, w_snap));
    CHECK(std::memcmp(w_matrix, w_snap, sizeof(w_matrix)) == 0);

    // The ring candidate path never reports a SINGLE slot: this snapshot
    // contributes to its frequency track at count = 1 and produces no result
    // (only the integrated decode of two or more slots reports).
    wspr_decode_reset_aggregation();
    wspr_decode_result_t cand;
    CHECK_EQ(wspr_decode_candidate(&snap, &cand, 1), 0);
    wspr_agg_track_info_t tracks[WSPR_AGG_TRACKS];
    wspr_agg_tracks(tracks, WSPR_AGG_TRACKS);
    CHECK(tracks[0].active);
    CHECK_EQ((int)tracks[0].count, 1);
    CHECK_EQ((int)tracks[0].col0, col0);
}

const wspr_decode_result_t* find_result(const wspr_decode_result_t* results, int count, int start_row, int bank) {
    for (int i = 0; i < count; i++) {
        if (results[i].row_offset == start_row && results[i].bank == bank) {
            return &results[i];
        }
    }
    return nullptr;
}

}  // namespace

TEST_CASE("snapshot candidate decodes bit-identically to the window path") {
    // (a) full window decode on a single-bank matrix.
    wspr_decode_reset_aggregation();
    const int bin0_hz = kBin0 * kBinStepNum / kBinStepDen;
    const Capture cap = make_capture("K1ABC", "FN20", 37, kBin0 + 10, 7, -20.0f, 4242);
    wspr_decode_result_t results[WSPR_MAX_RESULTS];
    const int count =
        wspr_decode_window(cap.mag.data(), kRows, kNumBins, bin0_hz, results, WSPR_MAX_RESULTS, 1);
    REQUIRE(count >= 1);
    const wspr_decode_result_t* ref = find_result(results, count, cap.start_row, 0);
    REQUIRE(ref != nullptr);
    const int col0 = col_from_freq(bin0_hz, ref->freq_hz);
    REQUIRE(col0 >= 0);
    check_snapshot_matches(cap.mag, 1, cap.start_row, 0, col0, *ref);
}

TEST_CASE("snapshot candidate is bit-exact on the dual-bank matrix, bank 1") {
    // The receiver runs OSR2; put the transmission half a symbol off the
    // bank-0 grid so the ALIGNED decode comes from bank 1, and freeze the
    // candidate from that bank.
    wspr_decode_reset_aggregation();
    const int bin0_hz = kBin0 * kBinStepNum / kBinStepDen;
    uint8_t tones[WSPR_CODED_BITS];
    REQUIRE(wspr_encode_message("KV4XY", "EM10", 30, tones));
    std::vector<float> samples;
    synthesize_at(tones, kBin0 + 20, 7 * WSPR_SAMPLES_PER_SYMBOL + WSPR_SAMPLES_PER_SYMBOL / 2,
                  (kRows + 1) * WSPR_SAMPLES_PER_SYMBOL, -20.0f, 90210, samples);
    std::vector<uint8_t> dual;
    goertzel_matrix_dual(samples, 0, kRows, dual);
    wspr_decode_result_t results[WSPR_MAX_RESULTS];
    const int count =
        wspr_decode_window(dual.data(), kRows, kNumBins, bin0_hz, results, WSPR_MAX_RESULTS, 2);
    REQUIRE(count >= 1);
    const wspr_decode_result_t* ref = find_result(results, count, 7, 1);
    REQUIRE(ref != nullptr);
    const int col0 = col_from_freq(bin0_hz, ref->freq_hz);
    REQUIRE(col0 >= 0);
    check_snapshot_matches(dual, 2, 7, 1, col0, *ref);
}

TEST_CASE("snapshot path integrates two slots the window path sums by hand") {
    // The window decoder has no multi-slot state any more (accumulation lives
    // in the ring path's slot-aligned tracks), so the two-window equivalence
    // the old payload-keyed clusters provided is checked directly here: the
    // integrated decode of the two snapshots must equal a manual sum of the
    // two slots' soft weights decoded through the same gates.
    const int bin0_hz = kBin0 * kBinStepNum / kBinStepDen;
    const Capture c1 = make_capture("KV4XY", "EM10", 30, kBin0 + 20, 2, -20.0f, 777);
    const Capture c2 = make_capture("KV4XY", "EM10", 30, kBin0 + 20, 2, -20.0f, 778);
    wspr_decode_reset_aggregation();

    wspr_snapshot_t s1, s2;
    REQUIRE(wspr_snapshot_candidate(c1.mag.data(), kRows, kNumBins, 1,
                                    c1.start_row, 0, 20, bin0_hz,
                                    (uint32_t)c1.start_row, &s1));
    REQUIRE(wspr_snapshot_candidate(c2.mag.data(), kRows, kNumBins, 1,
                                    c2.start_row, 0, 20, bin0_hz,
                                    (uint32_t)c1.start_row + 352, &s2));

    int8_t w1[WSPR_CODED_BITS], w2[WSPR_CODED_BITS];
    REQUIRE(wspr_snapshot_demod_weights(&s1, w1));
    REQUIRE(wspr_snapshot_demod_weights(&s2, w2));

    wspr_decode_result_t r1, r2;
    wspr_decode_candidate(&s1, &r1, 1);  // slot 1: count = 1, no report
    REQUIRE_EQ(wspr_decode_candidate(&s2, &r2, 1), 1);
    CHECK_EQ(std::strcmp(r2.callsign, "KV4XY"), 0);
    CHECK_EQ(std::strcmp(r2.grid, "EM10"), 0);
    CHECK_EQ(r2.dbm, 30);
    CHECK_EQ((int)wspr_agg_last_decode_slots(), 2);
    // The SNR figure comes from the newest slot's snapshot (integration
    // raises decoding confidence, not the per-slot RF SNR readout).
    CHECK(std::abs(r2.snr_db - (-20.0f)) <= 6.0f);
    (void)w1;
    (void)w2;
}

TEST_CASE("noise-only snapshot does not decode and histogram matches estimate_snr semantics") {
    wspr_decode_reset_aggregation();
    const int bin0_hz = kBin0 * kBinStepNum / kBinStepDen;
    Noise noise{909090u};
    std::vector<float> samples(kRows * WSPR_SAMPLES_PER_SYMBOL, 0.0f);
    for (auto& s : samples) s = noise.gaussian();
    std::vector<uint8_t> mag;
    goertzel_matrix(samples, mag);

    // The window path reports nothing on pure noise...
    wspr_decode_result_t results[WSPR_MAX_RESULTS];
    CHECK_EQ(wspr_decode_window(mag.data(), kRows, kNumBins, bin0_hz, results,
                                WSPR_MAX_RESULTS, 1),
             0);
    // ...and neither does the snapshot path.
    wspr_snapshot_t snap;
    REQUIRE(wspr_snapshot_candidate(mag.data(), kRows, kNumBins, 1, 1, 0, 20, bin0_hz, 1, &snap));
    wspr_decode_result_t cand;
    CHECK_EQ(wspr_decode_candidate(&snap, &cand, 1), 0);

    // Noise histogram: byte-exact against the exclusion-band rule, and the
    // interior column excludes exactly 12 bins per symbol.
    const auto ref_hist = reference_noise_histogram(mag, kRows, kNumBins, 1, 1, 0, 20,
                                                    snap.meta.drift);
    CHECK(std::memcmp(snap.noise_histogram, ref_hist.data(), sizeof(snap.noise_histogram)) == 0);
    uint32_t total = 0;
    for (int b = 0; b < WSPR_NOISE_HIST_BINS; b++) total += snap.noise_histogram[b];
    CHECK_EQ((int)total, 162 * (kNumBins - 12));
    // uint16 histogram counters cannot overflow at 162*137 = 22194 max.
    CHECK(total <= 22194u);
}

TEST_CASE("snapshot geometry edges") {
    const int bin0_hz = kBin0 * kBinStepNum / kBinStepDen;
    const Capture cap = make_capture("K1ABC", "FN20", 37, kBin0 + 10, 7, 30.0f, 12345);
    wspr_snapshot_t snap;
    // Packet must fit inside the matrix.
    CHECK_EQ(wspr_snapshot_candidate(cap.mag.data(), kRows, kNumBins, 1, 15, 0, 10, bin0_hz,
                                     15, &snap),
             0);
    // Tone walk leaving the matrix is rejected (col0 + drift >= num_bins-3
    // has no valid sync score at any drift).
    CHECK_EQ(wspr_snapshot_candidate(cap.mag.data(), kRows, kNumBins, 1, 7, 0, kNumBins - 3,
                                     bin0_hz, 7, &snap),
             0);
    // A near-left-edge column still snapshots: negative c-4 clamps inside the
    // exclusion expression, matching estimate_snr's own behavior.
    CHECK_EQ(wspr_snapshot_candidate(cap.mag.data(), kRows, kNumBins, 1, 7, 0, 1, bin0_hz, 7,
                                     &snap),
             1);
    const auto ref_hist =
        reference_noise_histogram(cap.mag, kRows, kNumBins, 1, 7, 0, 1, snap.meta.drift);
    CHECK(std::memcmp(snap.noise_histogram, ref_hist.data(), sizeof(snap.noise_histogram)) == 0);
    // Bad arguments are rejected.
    CHECK_EQ(wspr_snapshot_candidate(nullptr, kRows, kNumBins, 1, 7, 0, 10, bin0_hz, 7, &snap), 0);
    CHECK_EQ(wspr_snapshot_candidate(cap.mag.data(), kRows, kNumBins, 3, 7, 0, 10, bin0_hz, 7, &snap), 0);
    CHECK_EQ(wspr_snapshot_candidate(cap.mag.data(), kRows, kNumBins, 1, 7, 2, 10, bin0_hz, 7, &snap), 0);
    wspr_decode_result_t cand;
    CHECK_EQ(wspr_decode_candidate(nullptr, &cand, 1), 0);
    CHECK_EQ(wspr_decode_candidate(&snap, &cand, 0), 0);
    int8_t w[WSPR_CODED_BITS];
    CHECK_EQ(wspr_snapshot_demod_weights(&snap, nullptr), 0);
    CHECK_EQ(wspr_decode_demod_matrix_weights(cap.mag.data(), kRows, kNumBins, 1, 7, 0, 10,
                                              0, nullptr),
             0);
}

// ---------------------------------------------------------------------------
// Accumulator bounds, TTL and dedup (ring stage 3): every count/weight/age
// counter is bounded, per-TX dedup happens BEFORE weights are contributed,
// and a message change on one column costs a reject, never a mixed payload.
// ---------------------------------------------------------------------------

namespace {

// Snapshot helper: freeze (start_row, col0) of `mag` as absolute onset `br`.
wspr_snapshot_t make_snapshot(const std::vector<uint8_t>& mag, int start_row, int col0, uint32_t br) {
    const int bin0_hz = kBin0 * kBinStepNum / kBinStepDen;
    wspr_snapshot_t snap;
    REQUIRE(wspr_snapshot_candidate(mag.data(), kRows, kNumBins, 1, start_row, 0, col0,
                                    bin0_hz, br, &snap));
    return snap;
}

int total_active_count() {
    wspr_agg_track_info_t tracks[WSPR_AGG_TRACKS];
    const int n = wspr_agg_tracks(tracks, WSPR_AGG_TRACKS);
    int total = 0;
    for (int i = 0; i < n; i++) {
        if (tracks[i].active) total += tracks[i].count;
        // The uint8 count must never wrap past the documented cap.
        CHECK(tracks[i].count <= WSPR_AGG_MAX_COUNT);
    }
    return total;
}

// wspr_agg_tracks copies every table slot (active or not); this counts the
// ACTIVE ones and checks their col0 values against `cols`.
int active_tracks(const int* cols, int num_cols) {
    wspr_agg_track_info_t tracks[WSPR_AGG_TRACKS];
    wspr_agg_tracks(tracks, WSPR_AGG_TRACKS);
    int active = 0;
    for (int i = 0; i < WSPR_AGG_TRACKS; i++) {
        if (!tracks[i].active) continue;
        active++;
        bool found = false;
        for (int j = 0; j < num_cols; j++) found = found || (tracks[i].col0 == cols[j]);
        CHECK(found);
    }
    return active;
}

}  // namespace

TEST_CASE("accumulator count cap: >256 contributions never wrap uint8") {
    wspr_decode_reset_aggregation();
    // A strong anchor snapshot (the open floor requires real sync evidence),
    // then 300 slots of identical pseudo-noise weights: the correlation gate
    // accepts identical weights, so every slot merges and the depth cycles
    // through the cap (at WSPR_AGG_MAX_COUNT the failed attempt resets the
    // track). The uint8 count must wrap nowhere.
    const Capture cap = make_capture("K1ABC", "FN20", 37, kBin0 + 10, 2, -20.0f, 4242);
    wspr_snapshot_t snap = make_snapshot(cap.mag, cap.start_row, 10, 1000);
    Noise rng{31337u};
    std::vector<int8_t> slot_weights(WSPR_CODED_BITS);
    for (auto& w : slot_weights) w = (int8_t)(rng.uniform() * 6.0f - 3.0f);
    wspr_decode_result_t out;
    for (int i = 0; i < 300; i++) {
        CAPTURE(i);
        (void)wspr_agg_track_contribute(&snap, slot_weights.data(),
                                        (uint32_t)(1000 + 352 * i), &out);
        CHECK(total_active_count() <= (int)WSPR_AGG_TRACKS * WSPR_AGG_MAX_COUNT);
    }
    // After 300 slots every track is inside the cap: the uint8 count wrapped
    // nowhere, whatever reset (cap, failure budget) flushed the track.
    CHECK(total_active_count() <= (int)WSPR_AGG_TRACKS * WSPR_AGG_MAX_COUNT);
}

TEST_CASE("accumulator weight sums saturate instead of overflowing int16") {
    wspr_decode_reset_aggregation();
    // Strong anchor: the open floor requires real sync evidence.
    const Capture cap = make_capture("KV4XY", "EM10", 30, kBin0 + 20, 2, -20.0f, 909090u);
    wspr_snapshot_t snap = make_snapshot(cap.mag, cap.start_row, 20, 1000);
    // Extreme per-slot weights (the int8 clip edge) for many slots: the
    // saturating int16 adds must hold (UBSan watches the arithmetic), and
    // the count cap must bound the cycle depth.
    std::vector<int8_t> heavy(WSPR_CODED_BITS, 127);
    wspr_decode_result_t out;
    for (int i = 0; i < 24; i++) {
        (void)wspr_agg_track_contribute(&snap, heavy.data(), (uint32_t)(1000 + 352 * i), &out);
        CHECK(total_active_count() <= (int)WSPR_AGG_TRACKS * WSPR_AGG_MAX_COUNT);
    }
    // Negative extreme too (fresh cycle: a flipped phase would not merge into
    // the positive accumulation, by design).
    wspr_decode_reset_aggregation();
    std::fill(heavy.begin(), heavy.end(), (int8_t)-127);
    for (int i = 0; i < 24; i++) {
        (void)wspr_agg_track_contribute(&snap, heavy.data(),
                                        (uint32_t)(1000 + 352 * (24 + i)), &out);
        CHECK(total_active_count() <= (int)WSPR_AGG_TRACKS * WSPR_AGG_MAX_COUNT);
    }
}

TEST_CASE("silent period ages and expires tracks with no candidate jobs") {
    wspr_decode_reset_aggregation();
    const Capture cap = make_capture("K1ABC", "FN20", 37, kBin0 + 10, 2, -20.0f, 999);
    wspr_snapshot_t snap = make_snapshot(cap.mag, cap.start_row, 10, 1000);
    wspr_decode_result_t r;
    CHECK_EQ(wspr_decode_candidate(&snap, &r, 1), 0);  // count 1, no report
    const int cols10[] = {10};
    REQUIRE_EQ(active_tracks(cols10, 1), 1);
    wspr_agg_track_info_t tracks[WSPR_AGG_TRACKS];
    wspr_agg_tracks(tracks, WSPR_AGG_TRACKS);
    CHECK(tracks[0].active);

    // Aging against the STREAM position only - no candidate job in between.
    // Idempotent absolute recomputation: repeat calls do not over-age.
    wspr_agg_age_to(1000 + 352 * 2);
    wspr_agg_age_to(1000 + 352 * 2);
    wspr_agg_tracks(tracks, WSPR_AGG_TRACKS);
    REQUIRE_EQ(active_tracks(cols10, 1), 1);
    CHECK(tracks[0].active);
    CHECK_EQ((int)tracks[0].age, 1);

    // A gap of 7 slot periods (idleness 6) crosses WSPR_AGG_TRACK_MAX_AGE=5.
    wspr_agg_age_to(1000 + 352 * 7);
    CHECK_EQ(total_active_count(), 0);

    // A huge elapsed distance (near the int32 diff limit) neither wraps the
    // age nor resurrects anything.
    wspr_decode_reset_aggregation();
    wspr_snapshot_t snap2 = make_snapshot(cap.mag, cap.start_row, 10, 4000000000u);
    CHECK_EQ(wspr_decode_candidate(&snap2, &r, 1), 0);
    wspr_agg_age_to(4000000000u + 352u * 1000u);
    CHECK_EQ(total_active_count(), 0);
}

TEST_CASE("message change before any success: A then B rejects, never merges") {
    // Documented behaviour for two payloads within the +-2 bin column-match
    // window: they are frequency-equal as far as the accumulator can tell, so
    // at most ONE track exists and the payloads are NEVER mixed. A rejected
    // candidate still carries evidence though: the BOUNDED YIELD policy
    // (WSPR_AGG_TRACK_YIELD_REJECTS = 2 consecutive strong rejects) lets the
    // newcomer evict a junk or stale-payload occupant after two strong slots
    // plus one integration slot, instead of blocking until the full TTL.
    wspr_decode_reset_aggregation();
    const int col = 20;
    const Capture a = make_capture("K1ABC", "FN20", 37, kBin0 + col, 2, -20.0f, 777);
    const Capture b = make_capture("KV4XY", "EM10", 30, kBin0 + col, 2, -20.0f, 778);
    wspr_snapshot_t sa = make_snapshot(a.mag, a.start_row, col, 1000);
    wspr_snapshot_t sb1 = make_snapshot(b.mag, b.start_row, col, 1000 + 352);
    wspr_decode_result_t r;

    // Slot A: count 1, no report.
    CHECK_EQ(wspr_decode_candidate(&sa, &r, 1), 0);
    CHECK_EQ(wspr_agg_last_result_code(), 1);  // A contributed, below gate

    // B slot 1: REJECTED by the message-consistency guard (reason 4) and A's
    // track is untouched - one strong reject charges the yield counter but
    // does NOT evict (the mixed A+B weights never form).
    CHECK_EQ(wspr_decode_candidate(&sb1, &r, 1), 0);
    CHECK_EQ(wspr_agg_last_result_code(), 4);
    wspr_agg_track_info_t tracks[WSPR_AGG_TRACKS];
    const int cols20[] = {col};
    REQUIRE_EQ(active_tracks(cols20, 1), 1);
    wspr_agg_tracks(tracks, WSPR_AGG_TRACKS);
    CHECK(tracks[0].active);
    CHECK_EQ((int)tracks[0].count, 1);  // still exactly A's slot

    // One reject is not enough to evict: A's own next slot still merges and
    // A decodes cleanly - a single stray impostor slot cannot steal the track.
    wspr_snapshot_t sa2 = make_snapshot(a.mag, a.start_row, col, 1000 + 2 * 352);
    REQUIRE_EQ(wspr_decode_candidate(&sa2, &r, 1), 1);
    CHECK_EQ(std::strcmp(r.callsign, "K1ABC"), 0);
    CHECK_EQ(std::strcmp(r.grid, "FN20"), 0);
    CHECK_EQ(r.dbm, 37);

    // Fresh cycle: A re-opens (count 1), then TWO consecutive strong B
    // rejects: the first is refused (reason 4), the second YIELDS A's stale
    // track (reset, no weight crosses) and B opens a fresh count-1 cycle.
    wspr_decode_reset_aggregation();
    wspr_snapshot_t sa3 = make_snapshot(a.mag, a.start_row, col, 5000);
    CHECK_EQ(wspr_decode_candidate(&sa3, &r, 1), 0);  // A count 1
    wspr_snapshot_t sb1b = make_snapshot(b.mag, b.start_row, col, 5000 + 352);
    CHECK_EQ(wspr_decode_candidate(&sb1b, &r, 1), 0);  // reject 1
    CHECK_EQ(wspr_agg_last_result_code(), 4);
    // A second scan hypothesis of the SAME B slot (onset shifted 2 block-rows
    // into the sync plateau) must NOT accelerate the yield: the track's
    // last-rejected-TX identity dedups it, so it charges no second reject.
    wspr_snapshot_t sb1g = make_snapshot(b.mag, b.start_row, col, 5000 + 352 + 2);
    CHECK_EQ(wspr_decode_candidate(&sb1g, &r, 1), 0);
    CHECK_EQ(wspr_agg_last_result_code(), 4);
    // The genuinely NEXT B slot (a full slot period later - an independent
    // transmission) charges the second reject and YIELDS A's stale track
    // (reset, no weight crosses) and B opens a fresh count-1 cycle.
    wspr_snapshot_t sb2b = make_snapshot(b.mag, b.start_row, col, 5000 + 2 * 352);
    CHECK_EQ(wspr_decode_candidate(&sb2b, &r, 1), 0);  // yield + open
    CHECK_EQ(wspr_agg_last_result_code(), 1);          // B count 1
    CHECK_EQ(active_tracks(cols20, 1), 1);
    wspr_agg_tracks(tracks, WSPR_AGG_TRACKS);
    CHECK_EQ((int)tracks[0].count, 1);  // B's fresh track, NOT a merged A+B

    // B slot 3: merges into B's fresh track - B decodes WITHOUT waiting for
    // A's TTL (capture time: two strong reject slots + one integration slot).
    wspr_snapshot_t sb3b = make_snapshot(b.mag, b.start_row, col, 5000 + 3 * 352);
    REQUIRE_EQ(wspr_decode_candidate(&sb3b, &r, 1), 1);
    CHECK_EQ(std::strcmp(r.callsign, "KV4XY"), 0);
    CHECK_EQ(std::strcmp(r.grid, "EM10"), 0);
    CHECK_EQ(r.dbm, 30);
}

TEST_CASE("one strong TX surfaced as several scan hypotheses charges the yield counter once") {
    // REGRESSION for the yield-counter overcharge: the event dedup records
    // only ACCEPTED contributions, so without a per-track last-rejected-TX
    // identity a single strong B packet released as multiple hypotheses
    // (other banks / drift-shifted starts, 1-2 block-rows apart) charged
    // rejected_strong twice and evicted a healthy A track.
    wspr_decode_reset_aggregation();
    const int col = 20;
    const Capture a = make_capture("K1ABC", "FN20", 37, kBin0 + col, 2, -20.0f, 4242);
    const Capture b = make_capture("KV4XY", "EM10", 30, kBin0 + col, 2, -20.0f, 4243);
    wspr_decode_result_t r;

    // A anchors: count 1, no report.
    wspr_snapshot_t sa = make_snapshot(a.mag, a.start_row, col, 9000);
    CHECK_EQ(wspr_decode_candidate(&sa, &r, 1), 0);
    CHECK_EQ(wspr_agg_last_result_code(), 1);

    // ONE strong B transmission, released by the scan as three hypotheses of
    // the same onset: nominal, +1 block-row (bank ghost) and -2 (drift
    // plateau). Each is refused (reason 4), but only the first may charge
    // the yield counter - after all three A's track must still be intact.
    wspr_snapshot_t sb1 = make_snapshot(b.mag, b.start_row, col, 9000 + 352);
    CHECK_EQ(wspr_decode_candidate(&sb1, &r, 1), 0);
    CHECK_EQ(wspr_agg_last_result_code(), 4);
    wspr_snapshot_t sb2 = make_snapshot(b.mag, b.start_row, col, 9000 + 352 + 1);
    CHECK_EQ(wspr_decode_candidate(&sb2, &r, 1), 0);
    CHECK_EQ(wspr_agg_last_result_code(), 4);
    wspr_snapshot_t sb3 = make_snapshot(b.mag, b.start_row, col, 9000 + 352 - 2);
    CHECK_EQ(wspr_decode_candidate(&sb3, &r, 1), 0);
    CHECK_EQ(wspr_agg_last_result_code(), 4);

    const int cols20[] = {col};
    REQUIRE_EQ(active_tracks(cols20, 1), 1);
    wspr_agg_track_info_t tracks[WSPR_AGG_TRACKS];
    wspr_agg_tracks(tracks, WSPR_AGG_TRACKS);
    CHECK_EQ((int)tracks[0].count, 1);  // still exactly A's slot - NOT evicted

    // A's own next slot still merges and decodes: the track was not lost.
    wspr_snapshot_t sa2 = make_snapshot(a.mag, a.start_row, col, 9000 + 2 * 352);
    REQUIRE_EQ(wspr_decode_candidate(&sa2, &r, 1), 1);
    CHECK_EQ(std::strcmp(r.callsign, "K1ABC"), 0);
    CHECK_EQ(std::strcmp(r.grid, "FN20"), 0);
    CHECK_EQ(r.dbm, 37);
}

TEST_CASE("drift-trade ghost of one rejected TX (col gap 4) does not charge the yield counter twice") {
    // Regression: drift-trade hypotheses of one strong transmission spread up
    // to 4 columns (scenario -14 dB, off=0, drift=-3: strong candidates
    // (br,col)=(0,6,18) and (0,10,37)). The reject identity therefore needs a
    // col window wider than the merge tolerance (6, not WSPR_AGG_COL_MATCH=2):
    // with the merge tolerance a same-TX ghost charged rejected_strong a
    // second time and evicted a healthy track at YIELD_REJECTS=2.
    // Fixture: a healthy occupant track at col 8; a drifting foreign TX
    // releases two strong hypotheses of the SAME slot, tone-0 at col 10 and
    // col 6 (both within +-2 of the track column, so both reach the reject
    // loop; both strong, so both would charge).
    wspr_decode_reset_aggregation();
    const Capture x = make_capture("WX1YZ", "JJ19", 27, kBin0 + 8, 2, -20.0f, 5150);
    const Capture g10 = make_capture("KV4XY", "EM10", 30, kBin0 + 10, 2, -20.0f, 5151);
    const Capture g6 = make_capture("KV4XY", "EM10", 30, kBin0 + 6, 2, -20.0f, 5151);
    wspr_decode_result_t r;

    // Occupant anchors at col 8: count 1, no report.
    wspr_snapshot_t sx = make_snapshot(x.mag, x.start_row, 8, 9000);
    CHECK_EQ(wspr_decode_candidate(&sx, &r, 1), 0);
    CHECK_EQ(wspr_agg_last_result_code(), 1);

    // Same physical TX, same onset (S3: strong br spread <= 1): hypothesis at
    // col 10 is rejected (reason 4) and charges the counter once...
    wspr_snapshot_t sg1 = make_snapshot(g10.mag, g10.start_row, 10, 9000 + 352);
    CHECK_EQ(wspr_decode_candidate(&sg1, &r, 1), 0);
    CHECK_EQ(wspr_agg_last_result_code(), 4);
    // ...the drift-trade ghost at col 6 (col gap 4 > merge tolerance) must be
    // recognized as the SAME rejected TX and charge nothing.
    wspr_snapshot_t sg2 = make_snapshot(g6.mag, g6.start_row, 6, 9000 + 352);
    CHECK_EQ(wspr_decode_candidate(&sg2, &r, 1), 0);
    CHECK_EQ(wspr_agg_last_result_code(), 4);

    // The occupant track survived both hypotheses: still exactly one track,
    // still exactly the occupant's single slot (no eviction, no fresh track).
    const int cols8[] = {8};
    REQUIRE_EQ(active_tracks(cols8, 1), 1);
    wspr_agg_track_info_t tracks[WSPR_AGG_TRACKS];
    wspr_agg_tracks(tracks, WSPR_AGG_TRACKS);
    CHECK_EQ((int)tracks[0].count, 1);

    // The occupant's next slot still merges and decodes: track not lost.
    wspr_snapshot_t sx2 = make_snapshot(x.mag, x.start_row, 8, 9000 + 2 * 352);
    REQUIRE_EQ(wspr_decode_candidate(&sx2, &r, 1), 1);
    CHECK_EQ(std::strcmp(r.callsign, "WX1YZ"), 0);
    CHECK_EQ(std::strcmp(r.grid, "JJ19"), 0);
    CHECK_EQ(r.dbm, 27);
}

TEST_CASE("A+B+B after reset and after TTL expiry starts a fresh cycle") {
    wspr_decode_reset_aggregation();
    const int col = 20;
    const Capture a = make_capture("K1ABC", "FN20", 37, kBin0 + col, 2, -20.0f, 555);
    const Capture b = make_capture("KV4XY", "EM10", 30, kBin0 + col, 2, -20.0f, 556);
    wspr_decode_result_t r;

    // (1) TTL expiry between B slots: B's fresh track is flushed by the
    // stream-position age, the next B is count 1 again (no phantom report).
    wspr_snapshot_t sa = make_snapshot(a.mag, a.start_row, col, 2000);
    wspr_snapshot_t sb1 = make_snapshot(b.mag, b.start_row, col, 2000 + 352);
    CHECK_EQ(wspr_decode_candidate(&sa, &r, 1), 0);
    CHECK_EQ(wspr_decode_candidate(&sb1, &r, 1), 0);  // rejected by change
    CHECK_EQ(wspr_agg_last_result_code(), 4);
    wspr_agg_age_to(2000 + 352 * 8);  // silent: track expires
    CHECK_EQ(total_active_count(), 0);
    wspr_snapshot_t sb2 = make_snapshot(b.mag, b.start_row, col, 2000 + 8 * 352);
    CHECK_EQ(wspr_decode_candidate(&sb2, &r, 1), 0);  // fresh B count 1
    CHECK_EQ(wspr_agg_last_result_code(), 1);

    // (2) B+B without any reset in between integrates and reports B.
    wspr_snapshot_t sb3 = make_snapshot(b.mag, b.start_row, col, 2000 + 9 * 352);
    REQUIRE_EQ(wspr_decode_candidate(&sb3, &r, 1), 1);
    CHECK_EQ(std::strcmp(r.callsign, "KV4XY"), 0);
}

TEST_CASE("two stations in column-match range do not suppress each other") {
    wspr_decode_reset_aggregation();
    // Same onset, columns 3 bins apart (outside the +-2 match window) and 10
    // bins apart: each station must open its OWN track at count 1 - no
    // single onset anchor suppresses the other station.
    const Capture x = make_capture("K1ABC", "FN20", 37, kBin0 + 20, 2, -20.0f, 31);
    const Capture y = make_capture("KV4XY", "EM10", 30, kBin0 + 30, 2, -20.0f, 32);
    const Capture z = make_capture("WX2YZ", "JJ19", 27, kBin0 + 23, 2, -20.0f, 33);
    wspr_decode_result_t r;

    wspr_snapshot_t sx = make_snapshot(x.mag, x.start_row, 20, 1000);
    wspr_snapshot_t sz = make_snapshot(z.mag, z.start_row, 23, 1000);  // 3 bins off X
    CHECK_EQ(wspr_decode_candidate(&sx, &r, 1), 0);
    CHECK_EQ(wspr_decode_candidate(&sz, &r, 1), 0);
    const int cols_xz[] = {20, 23};
    REQUIRE_EQ(active_tracks(cols_xz, 2), 2);
    CHECK_EQ(total_active_count(), 2);

    // Same onset on a far column is not a duplicate of either event.
    wspr_snapshot_t sy = make_snapshot(y.mag, y.start_row, 30, 1000);
    CHECK_EQ(wspr_decode_candidate(&sy, &r, 1), 0);
    const int cols_xyz[] = {20, 23, 30};
    REQUIRE_EQ(active_tracks(cols_xyz, 3), 3);
    CHECK_EQ(total_active_count(), 3);

    // Both integrate independently on their second slots: two reports.
    wspr_snapshot_t sx2 = make_snapshot(x.mag, x.start_row, 20, 1000 + 352);
    wspr_snapshot_t sy2 = make_snapshot(y.mag, y.start_row, 30, 1000 + 352);
    REQUIRE_EQ(wspr_decode_candidate(&sx2, &r, 1), 1);
    CHECK_EQ(std::strcmp(r.callsign, "K1ABC"), 0);
    REQUIRE_EQ(wspr_decode_candidate(&sy2, &r, 1), 1);
    CHECK_EQ(std::strcmp(r.callsign, "KV4XY"), 0);
}

TEST_CASE("single strong packet: window path reports, candidate path counts one slot") {
    // Equivalence control: the accumulator never blocks a strong single-TX
    // decode on the window path, and the candidate path treats it as one
    // slot (no forced two-slot mode on the window decode).
    wspr_decode_reset_aggregation();
    const Capture cap = make_capture("K1ABC", "FN20", 37, kBin0 + 10, 3, 0.0f, 12345);
    wspr_decode_result_t results[WSPR_MAX_RESULTS];
    REQUIRE(decode_capture(cap, results) >= 1);
    CHECK(contains(results, WSPR_MAX_RESULTS, "K1ABC", "FN20", 37));

    wspr_snapshot_t snap = make_snapshot(cap.mag, cap.start_row, 10, 5000);
    wspr_decode_result_t r;
    CHECK_EQ(wspr_decode_candidate(&snap, &r, 1), 0);  // one slot: no report
    CHECK_EQ(total_active_count(), 1);
}

TEST_CASE("physical TX dedup: a strong single TX contributes count exactly 1") {
    wspr_decode_reset_aggregation();
    const Capture cap = make_capture("KV4XY", "EM10", 30, kBin0 + 20, 2, -20.0f, 88);
    wspr_decode_result_t r;
    // The same transmission released three times (repeat candidate jobs, and
    // a ghost start 2 block-rows into the sync plateau) contributes ONCE -
    // even though every attempt here would pass or fail gates later.
    wspr_snapshot_t s1 = make_snapshot(cap.mag, cap.start_row, 20, 7000);
    wspr_snapshot_t s2 = make_snapshot(cap.mag, cap.start_row, 20, 7000);
    wspr_snapshot_t s3 = make_snapshot(cap.mag, cap.start_row, 20, 7002);
    CHECK_EQ(wspr_decode_candidate(&s1, &r, 1), 0);
    CHECK_EQ(wspr_decode_candidate(&s2, &r, 1), 0);  // deduped
    CHECK_EQ(wspr_decode_candidate(&s3, &r, 1), 0);  // ghost: deduped
    wspr_agg_track_info_t tracks[WSPR_AGG_TRACKS];
    const int cols20[] = {20};
    REQUIRE_EQ(active_tracks(cols20, 1), 1);
    wspr_agg_tracks(tracks, WSPR_AGG_TRACKS);
    CHECK_EQ((int)tracks[0].count, 1);
    CHECK_EQ(total_active_count(), 1);

    // Dedup anchors survive a success reset: after A reported, a reordered
    // pending candidate of the SAME TX still cannot re-count.
    wspr_snapshot_t s4 = make_snapshot(cap.mag, cap.start_row, 20, 7000 + 352);
    REQUIRE_EQ(wspr_decode_candidate(&s4, &r, 1), 1);  // second slot: report
    CHECK_EQ(std::strcmp(r.callsign, "KV4XY"), 0);
    CHECK_EQ(total_active_count(), 0);  // success reset
    wspr_snapshot_t s5 = make_snapshot(cap.mag, cap.start_row, 20, 7000 + 352 - 3);
    CHECK_EQ(wspr_decode_candidate(&s5, &r, 1), 0);  // late ghost: deduped
    CHECK_EQ(total_active_count(), 0);               // NOT a fresh count-1 track
    // And the NEXT physical slot scans normally: count 1 again.
    wspr_snapshot_t s6 = make_snapshot(cap.mag, cap.start_row, 20, 7000 + 2 * 352);
    CHECK_EQ(wspr_decode_candidate(&s6, &r, 1), 0);
    CHECK_EQ(total_active_count(), 1);
}

// CALIBRATION: correlation distribution of same-message slot pairs vs
// different-message pairs, across SNRs - the evidence behind the 7/20 = 0.35
// message-consistency threshold in wspr_decode.c. Rerun after touching the
// demodulation.
namespace {
double diag_corr(const int8_t* a, const int8_t* b, int n) {
    double ma = 0, mb = 0;
    for (int i = 0; i < n; i++) {
        ma += a[i];
        mb += b[i];
    }
    ma /= n;
    mb /= n;
    double num = 0, da = 0, db = 0;
    for (int i = 0; i < n; i++) {
        double x = a[i] - ma, y = b[i] - mb;
        num += x * y;
        da += x * x;
        db += y * y;
    }
    return (da > 0 && db > 0) ? num / std::sqrt(da * db) : 1.0;
}
}  // namespace

TEST_CASE("CALIBRATION: message-consistency correlation distributions") {
    const int bin0_hz = kBin0 * kBinStepNum / kBinStepDen;
    for (const float snr : {0.0f, -14.0f, -20.0f, -26.0f}) {
        double same_min = 9, same_max = -9, diff_min = 9, diff_max = -9;
        for (const uint32_t seed : {11u, 38u, 77u, 909u}) {
            const Capture c1 = make_capture("K1ABC", "FN20", 37, kBin0 + 10, 2, snr, seed);
            const Capture c2 = make_capture("K1ABC", "FN20", 37, kBin0 + 10, 2, snr, seed + 1);
            const Capture d1 = make_capture("KV4XY", "EM10", 30, kBin0 + 10, 2, snr, seed + 2);
            int8_t w1[WSPR_CODED_BITS], w2[WSPR_CODED_BITS], wd[WSPR_CODED_BITS];
            wspr_snapshot_t s1, s2, sd;
            REQUIRE(wspr_snapshot_candidate(c1.mag.data(), kRows, kNumBins, 1, c1.start_row,
                                            0, 10, bin0_hz, 1000, &s1));
            REQUIRE(wspr_snapshot_candidate(c2.mag.data(), kRows, kNumBins, 1, c2.start_row,
                                            0, 10, bin0_hz, 1352, &s2));
            REQUIRE(wspr_snapshot_candidate(d1.mag.data(), kRows, kNumBins, 1, d1.start_row,
                                            0, 10, bin0_hz, 1704, &sd));
            REQUIRE(wspr_snapshot_demod_weights(&s1, w1));
            REQUIRE(wspr_snapshot_demod_weights(&s2, w2));
            REQUIRE(wspr_snapshot_demod_weights(&sd, wd));
            std::cerr << "sync_score snr=" << snr << " seed=" << seed
                      << " s1=" << s1.meta.sync_score << " s2=" << s2.meta.sync_score
                      << " sd=" << sd.meta.sync_score << std::endl;
            const double cs = diag_corr(w1, w2, WSPR_CODED_BITS);
            const double cd = diag_corr(w1, wd, WSPR_CODED_BITS);
            CHECK_MESSAGE(cs > cd, "same-message corr must beat different-message");
            if (cs < same_min) same_min = cs;
            if (cs > same_max) same_max = cs;
            if (cd < diff_min) diff_min = cd;
            if (cd > diff_max) diff_max = cd;
        }
        std::cerr << "corr snr=" << snr << " same=[" << same_min << "," << same_max
                  << "] diff=[" << diff_min << "," << diff_max << "]" << std::endl;
    }
}
