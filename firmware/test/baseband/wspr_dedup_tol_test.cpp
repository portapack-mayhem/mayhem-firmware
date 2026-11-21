/*
 * Copyright (C) 2026 Ivan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

/*
 * DEDUP TOLERANCE STUDY (S3, FINAL_REVIEW_ROUND3 2026-11-17).
 *
 * Pure MEASUREMENT diagnostic, no expectations on existing logic:
 * feeds the canonical two-slot streaming fixtures (offsets 0/37/77/113 s,
 * drift -3..+3, SNR -14/-20/-26 dB) through the exact firmware decode-thread
 * loop and records EVERY candidate released by wspr_ring_scan_poll
 * (start_br, col, excess, drift, bank) to a CSV file (env WSPR_DEDUP_CSV,
 * default dedup_candidates.csv), tagged with the physical TX slot it
 * belongs to (nominal block-row +-6; others are junk).
 *
 * Purpose: measure the within-one-TX spread of candidate onsets and columns
 * to validate WSPR_AGG_REJECT_TX_TOL = 4 (and ConfirmTable TX_BR_TOLERANCE)
 * against the accepted-weights slot tolerance of 20. Any CHECK here is a
 * reporting assertion on the MEASUREMENT itself, not on receiver behaviour.
 */

#ifndef DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#endif
#include <doctest.h>

#include "../../baseband/wspr_portapack.h"

#include "wspr_constants.h"
#include "wspr_decode.h"
#include "wspr_encode.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr int kSynthBaseBin = 956 + 10;                          // column 10 of the matrix
constexpr uint32_t kSamplesPerSymbol = WSPR_SAMPLES_PER_SYMBOL;  // 8192
constexpr uint64_t kSlotSamples = 120ull * WSPR_SAMPLE_RATE;

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
        if (u1 < 1e-9f) u1 = 1e-9f;
        return std::sqrt(-2.0f * std::log(u1)) * std::cos(6.2831853f * uniform());
    }
};

void synthesize_stream(const uint8_t* tones, uint64_t start_sample, size_t num_samples, float snr_db, uint32_t seed, std::vector<float>& out, int drift = 0) {
    const float snr6 = std::pow(10.0f, (snr_db + 26.2f) / 10.0f);
    const float amp = std::sqrt(4.0f * snr6 * 6.0f / 12000.0f);
    Noise noise{seed};
    out.assign(num_samples, 0.0f);
    const float two_pi = 6.2831853f;
    const float phase_step = two_pi / (float)kSamplesPerSymbol;
    float phase = 0.0f;
    for (int n = 0; n < WSPR_CODED_BITS; n++) {
        const float f =
            (float)(kSynthBaseBin + tones[n] + (drift * n) / (WSPR_CODED_BITS - 1));
        const uint64_t begin = start_sample + (uint64_t)n * kSamplesPerSymbol;
        if (begin >= num_samples) break;
        const size_t len =
            (size_t)std::min<uint64_t>(kSamplesPerSymbol, num_samples - begin);
        for (size_t i = 0; i < len; i++) {
            out[begin + i] += amp * std::sin(phase + f * phase_step * (float)i);
        }
        phase = std::fmod(phase + f * phase_step * (float)kSamplesPerSymbol, two_pi);
    }
    for (size_t i = 0; i < out.size(); i++) {
        out[i] += noise.gaussian();
    }
}

void add_tx(std::vector<float>& stream, const uint8_t* tones, uint64_t start_sample, float snr_db, uint32_t seed, int drift = 0) {
    if (start_sample >= stream.size()) return;
    std::vector<float> shifted;
    synthesize_stream(tones, start_sample, stream.size(), snr_db, seed, shifted, drift);
    for (size_t i = (size_t)start_sample; i < stream.size(); i++) stream[i] += shifted[i];
}

std::vector<float> two_slot_stream(const uint8_t* tones, uint64_t offset_s, float snr_db, uint32_t seed, int drift = 0) {
    const uint64_t offset = offset_s * WSPR_SAMPLE_RATE;
    const size_t len = (size_t)offset + (size_t)(2 * kSlotSamples) +
                       (WSPR_CODED_BITS + 8) * kSamplesPerSymbol;
    std::vector<float> samples;
    synthesize_stream(tones, offset, len, snr_db, seed, samples, drift);
    add_tx(samples, tones, offset + kSlotSamples, snr_db, seed + 1, drift);
    return samples;
}

// One released candidate, with the physical TX slot tag (-1 = junk).
struct CandRecord {
    int scenario;
    float snr_db;
    uint64_t offset_s;
    int synth_drift;
    int slot;  // 0 / 1, or -1 junk
    uint32_t start_br;
    uint32_t nominal_br;
    int16_t col;
    int16_t excess;
    int8_t drift;
    uint32_t released_at_br;
    int demod_ok;  // snapshot+demod succeeded (weights produced)
};

struct CsvOut {
    std::ofstream f;
    explicit CsvOut(const char* path)
        : f(path) {
        f << "scenario,snr_db,offset_s,synth_drift,slot,start_br,nominal_br,col,"
             "excess,drift,released_at_br,demod_ok\n";
    }
    void add(const CandRecord& r) {
        f << r.scenario << ',' << r.snr_db << ',' << r.offset_s << ','
          << r.synth_drift << ',' << r.slot << ',' << r.start_br << ','
          << r.nominal_br << ',' << r.col << ',' << r.excess << ','
          << (int)r.drift << ',' << r.released_at_br << ',' << r.demod_ok << '\n';
    }
};

// Lean firmware decode-thread loop: feed -> scan poll -> snapshot -> demod.
// No fault injection; every released candidate is recorded.
struct MeasRx {
    wspr_portapack_state_t st{};
    wspr_ring_scan_state_t scan{};
    std::vector<CandRecord>* out = nullptr;
    const CandRecord* proto = nullptr;  // scenario fields to stamp

    MeasRx() {
        REQUIRE(wspr_portapack_init(&st));
        wspr_ring_scan_init(&scan);
        wspr_decode_reset_aggregation();
    }

    void feed(const std::vector<float>& samples) {
        for (const float s : samples) {
            if (!wspr_portapack_feed_sample(&st, s)) continue;
            while (true) {
                uint32_t br = wspr_portapack_total_br(&st);
                uint32_t start_br = 0;
                int16_t col = 0, excess = 0;
                int8_t drift = 0;
                const int have =
                    wspr_ring_scan_poll(wspr_portapack_matrix(), wspr_portapack_bins(), WSPR_RX_BLOCK_ROWS, br, &scan, WSPR_RING_SCAN_MAX_STARTS_PER_POLL, [](void* p) { return ((MeasRx*)p)->cursor(); }, this, &start_br, &col, &excess, &drift);
                if (!have) {
                    if ((int32_t)(br - scan.next_scan_br) < WSPR_CODED_BITS * 2 - 1) break;
                    continue;
                }
                CandRecord r = *proto;
                r.start_br = start_br;
                r.col = col;
                r.excess = excess;
                r.drift = drift;
                r.released_at_br = br;
                r.slot = -1;
                const uint32_t nom[2] = {
                    (uint32_t)(2u * (proto->offset_s * WSPR_SAMPLE_RATE / kSamplesPerSymbol)),
                    (uint32_t)(2u * ((proto->offset_s * WSPR_SAMPLE_RATE + kSlotSamples) /
                                     kSamplesPerSymbol))};
                for (int k = 0; k < 2; k++) {
                    const int32_t d = (int32_t)(start_br - nom[k]);
                    if (d >= -6 && d <= 6) {
                        r.slot = k;
                        r.nominal_br = nom[k];
                        break;
                    }
                }
                // Snapshot + demod like the firmware, to mark real completions.
                wspr_snapshot_t snap;
                uint32_t br_now = wspr_portapack_total_br(&st);
                r.demod_ok = 0;
                int snap_fail = WSPR_SNAP_OK;
                if (wspr_ring_snapshot_candidate(wspr_portapack_matrix(), wspr_portapack_bins(), WSPR_RX_BLOCK_ROWS, br_now, start_br, col, wspr_portapack_bin0_hz(), drift, excess, &snap, [](void* p) { return ((MeasRx*)p)->cursor(); }, this, &snap_fail)) {
                    int8_t sym_weight[WSPR_CODED_BITS];
                    if (wspr_snapshot_demod_weights(&snap, sym_weight)) {
                        r.demod_ok = 1;
                        wspr_decode_result_t integrated;
                        wspr_agg_track_contribute(&snap, sym_weight, start_br,
                                                  &integrated);
                    }
                }
                if (out) out->push_back(r);
            }
        }
    }

    uint32_t cursor() { return wspr_portapack_total_br(&st); }
};

constexpr const char* kCallsign = "K1ABC";
constexpr const char* kGrid = "FN20";

// Same figure as WSPR_AGG_OPEN_EXCESS_MIN in wspr_decode.c (file-static there):
// candidates at or above this grade would charge the yield counter.
constexpr int16_t kOpenExcessMin = 10;

std::vector<CandRecord> csv_rows;

void run_sweep(float snr_db, int scenario_base, CsvOut& csv) {
    uint8_t tones[WSPR_CODED_BITS];
    REQUIRE(wspr_encode_message(kCallsign, kGrid, 37, tones));
    int scenario = scenario_base;
    for (const uint64_t offset_s : {0ull, 37ull, 77ull, 113ull}) {
        for (int drift = -3; drift <= 3; drift++) {
            CandRecord proto{};
            proto.scenario = scenario;
            proto.snr_db = snr_db;
            proto.offset_s = offset_s;
            proto.synth_drift = drift;
            const auto t0 = std::chrono::steady_clock::now();
            MeasRx rx;
            rx.out = &csv_rows;
            rx.proto = &proto;
            rx.feed(two_slot_stream(tones, offset_s, snr_db,
                                    3100u + (uint32_t)scenario, drift));
            const auto t1 = std::chrono::steady_clock::now();
            const double secs =
                std::chrono::duration<double>(t1 - t0).count();
            size_t tagged = 0, strong = 0;
            for (const auto& r : csv_rows) {
                csv.add(r);
                if (r.slot >= 0) tagged++;
                if (r.slot >= 0 && r.excess >= kOpenExcessMin) strong++;
            }
            std::cerr << "scenario " << scenario << " snr=" << snr_db
                      << " off=" << offset_s << " drift=" << drift
                      << " candidates=" << tagged << " strong=" << strong
                      << " (" << secs << " s)\n";
            csv_rows.clear();
            scenario++;
        }
    }
}

}  // namespace

// Per-SNR test cases so ctest/doctest -tc filters can shard the sweep across
// parallel processes. Naming is descriptive of the MEASUREMENT, not an
// expectation of receiver behaviour.
TEST_CASE("DEDUP sweep: snr -14 dB") {
    const char* path = std::getenv("WSPR_DEDUP_CSV");
    CsvOut csv(path ? path : "dedup_candidates.csv");
    run_sweep(-14.0f, 0, csv);
}

TEST_CASE("DEDUP sweep: snr -20 dB") {
    const char* path = std::getenv("WSPR_DEDUP_CSV");
    CsvOut csv(path ? path : "dedup_candidates.csv");
    run_sweep(-20.0f, 28, csv);
}

TEST_CASE("DEDUP sweep: snr -26 dB") {
    const char* path = std::getenv("WSPR_DEDUP_CSV");
    CsvOut csv(path ? path : "dedup_candidates.csv");
    run_sweep(-26.0f, 56, csv);
}
