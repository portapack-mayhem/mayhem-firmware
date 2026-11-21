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
 * End-to-end STREAMING tests of the ring receiver (OSR2) with the SLOT-ALIGNED
 * weight accumulator: audio is fed sample by sample through
 * wspr_portapack_feed_sample (never a prepared matrix), and after every
 * completed block-row the exact scan/snapshot/demod/contribute step the
 * firmware decode thread runs is executed inline:
 *
 *   wspr_ring_scan_poll -> wspr_ring_snapshot_candidate
 *                       -> wspr_snapshot_demod_weights
 *                       -> wspr_agg_track_contribute
 *
 * Reporting semantics: a SINGLE slot never reports (the per-window stack
 * decode at low SNR decodes into unreliable impostor payloads - the defect
 * the accumulator replaces). A station's slot weights integrate in its
 * frequency track, and exactly one INTEGRATED decode reports once the track
 * holds two slots; the track then resets, so a message change starts a fresh
 * cycle and one message cannot report twice off the same evidence.
 *
 * Cases: two slots of one -14 dB signal at arbitrary capture phases decode
 * into exactly ONE report each time (K1ABC, integration depth 2); a single
 * slot reports nothing but holds count = 1; two same-start stations 100
 * columns apart produce two independent integrated reports with no
 * cross-talk; a message change after a successful decode does not re-report
 * the old message; drift reports still work; and a 60-minute noise-only feed
 * produces zero reports and leaves the track table clean. A final case keeps
 * the uint32 wrap coverage: two slots around the counter seam still integrate
 * and report.
 *
 * The scan phase also measures the per-start sync-scan cost (host wall clock)
 * for the CPU budget report.
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
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr int kFirstBin = 956;
constexpr int kSynthBaseBin = kFirstBin + 10;  // column 10 of the matrix

constexpr uint32_t kSamplesPerSymbol = WSPR_SAMPLES_PER_SYMBOL;  // 8192 @ 12 kHz

// One 120 s WSPR slot in samples; the slot-to-slot block-row gap is 351 or 352
// (1440000 / 4096 = 351.5625).
constexpr uint64_t kSlotSamples = 120ull * WSPR_SAMPLE_RATE;

// Deterministic Gaussian noise.
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

// Cheap uniform noise for the long noise-only feed (no log/cos per sample).
struct FastNoise {
    uint32_t state;
    explicit FastNoise(uint32_t seed)
        : state{seed ? seed : 1u} {}
    float next_sample() {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return ((float)(state >> 8) / (float)(1u << 23)) - 1.0f;
    }
};

// Synthesize num_samples of 12 kHz audio: one WSPR transmission starting at
// absolute sample start_sample, tone n on base_bin + tones[n], plus
// unit-variance white noise. snr_db is the WSJT-X display figure (2500 Hz
// noise reference), so the tone amplitude is
// A = sqrt(4 * snr6 * 6 / 12000) with snr6 the occupied-band (6 Hz) ratio.
// The drift walk matches the decoder's sym_col expression (integer division,
// truncation toward zero).
void synthesize_stream_bin(const uint8_t* tones, int base_bin, uint64_t start_sample, size_t num_samples, float snr_db, uint32_t seed, std::vector<float>& out, int drift = 0) {
    const float snr6 = std::pow(10.0f, (snr_db + 26.2f) / 10.0f);
    const float amp = std::sqrt(4.0f * snr6 * 6.0f / 12000.0f);
    Noise noise{seed};
    out.assign(num_samples, 0.0f);

    const float two_pi = 6.2831853f;
    const float phase_step = two_pi / (float)kSamplesPerSymbol;
    float phase = 0.0f;
    for (int n = 0; n < WSPR_CODED_BITS; n++) {
        const float f =
            (float)(base_bin + tones[n] + (drift * n) / (WSPR_CODED_BITS - 1));
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

// Same, on the standard synth base bin (column 10).
void synthesize_stream(const uint8_t* tones, uint64_t start_sample, size_t num_samples, float snr_db, uint32_t seed, std::vector<float>& out, int drift = 0) {
    synthesize_stream_bin(tones, kSynthBaseBin, start_sample, num_samples, snr_db,
                          seed, out, drift);
}

// ADD one transmission on top of an existing stream at start_sample offset, with
// its own noise seed - the second slot of a repeated message sees independent
// noise.
void add_tx(std::vector<float>& stream, const uint8_t* tones, uint64_t start_sample, float snr_db, uint32_t seed, int drift = 0) {
    if (start_sample >= stream.size()) return;
    std::vector<float> shifted;
    synthesize_stream(tones, start_sample, stream.size(), snr_db, seed, shifted, drift);
    for (size_t i = (size_t)start_sample; i < stream.size(); i++) stream[i] += shifted[i];
}

// ADD one transmission on a shifted base bin (a second station's frequency).
void add_tx_bin(std::vector<float>& stream, const uint8_t* tones, int base_bin, uint64_t start_sample, float snr_db, uint32_t seed) {
    if (start_sample >= stream.size()) return;
    const size_t len = stream.size() - (size_t)start_sample;
    std::vector<float> tx;
    synthesize_stream_bin(tones, base_bin, 0, len, snr_db, seed, tx);
    for (size_t i = 0; i < len; i++) stream[(size_t)start_sample + i] += tx[i];
}

struct DecodeRecord {
    uint32_t start_br;       // absolute block-row id the candidate claimed
    uint32_t decoded_at_br;  // ring counter when the decode ran
    int16_t col;
    int16_t excess;
    wspr_decode_result_t result;
};

// Debug record of a contributed (non-reporting) candidate, for diagnosis.
struct ContributionRecord {
    uint32_t start_br;
    int16_t col;
    int8_t drift;
    int8_t bank;
    std::vector<int8_t> weights;
    int reason;  // wspr_agg_last_result_code()
    int16_t excess;
};

// The firmware decode thread, inline: feed -> on every completed block-row drain the
// ring scan, snapshot any candidate, demod its weights, contribute them to the
// slot-aligned track accumulator. Capture never pauses.
struct RingRx {
    wspr_portapack_state_t st{};
    wspr_ring_scan_state_t scan{};
    std::vector<DecodeRecord> decodes;
    std::vector<ContributionRecord> contribs;
    uint64_t scan_calls = 0;
    uint64_t scan_ns = 0;  // host wall clock inside scan polls (accumulated)
    uint32_t last_excess = 0;

    // ---- Fault injection (producer preemption) ----
    // >0: after a candidate is RELEASED by the scan, advance the producer by this many
    // SYMBOL rows of WORKER SLEEP after a candidate release: the outer feed loop
    // keeps feeding the producer with the REAL stream samples for that long (capture
    // never pauses), but the worker does not poll - exactly the preemption model of
    // the firmware, where a seconds-long decode lets the producer run ahead ON THE
    // SAME stream instead of injecting foreign rows. Armed when a candidate is
    // released, consumed by the next feed() samples.
    int fault_rows_after_release = 0;
    uint64_t worker_sleep_samples_left = 0;
    // Only releases at least this strong arm the sleep: the firmware worker is
    // preempted for FEC-length stretches by REAL candidates (the sequential decode),
    // not by the microsecond snapshot+contribute of noise completions. Junk
    // releases without the arm keep the slot cadence intact, like on air.
    static constexpr int16_t kFaultMinExcess = 15;
    // A released candidate whose snapshot is deferred by the preemption sleep.
    struct Deferred {
        uint32_t br;
        int16_t col;
        int16_t excess;
        int8_t drift;
        bool valid = false;
    } deferred{};
    // Mid-snapshot preemption: advance the producer by this many SYMBOL rows on the
    // Nth live-cursor read INSIDE wspr_ring_snapshot_candidate (1-based, 0 disables),
    // so the overwrite happens between the guarded row copies.
    int fault_rows_mid_snapshot = 0;
    int fault_trigger_cursor_call = 0;
    int cursor_calls = 0;
    // Mid-SCAN preemption: advance the producer by this many SYMBOL rows on the Nth
    // live-cursor read INSIDE the scan bracket of wspr_ring_scan_start (1-based, 0
    // disables) - i.e. DURING the correlation, not merely between poll jobs. The
    // bracket fires on the 2nd cursor read of each scanned start (entry + exit), so
    // trigger = 2 overwrites the ring while the correlation pass is running.
    int fault_rows_mid_scan = 0;
    int fault_trigger_scan_cursor_call = 0;
    int scan_cursor_calls = 0;
    int last_snap_fail = WSPR_SNAP_OK;
    uint32_t snap_ok_count = 0;

    RingRx() {
        REQUIRE(wspr_portapack_init(&st));
        wspr_ring_scan_init(&scan);
        // The track table is file-static and shared by every test case in the
        // process; each scenario must start from a clean table.
        wspr_decode_reset_aggregation();
    }

    // The LIVE cursor accessor handed to wspr_ring_snapshot_candidate, mirroring the
    // firmware's wspr_ring_live_cursor(): reads the producer counter through the
    // portapack accessor every time (never a latched copy). When the mid-snapshot
    // fault is armed, the producer is advanced at the trigger call - the writer
    // "racing" the reader.
    uint32_t live_cursor() {
        ++cursor_calls;
        if (fault_rows_mid_snapshot > 0 && cursor_calls == fault_trigger_cursor_call) {
            feed_noise_symbol_rows((size_t)fault_rows_mid_snapshot);
            fault_rows_mid_snapshot = 0;  // fire once
        }
        return wspr_portapack_total_br(&st);
    }

    // The LIVE cursor accessor handed to wspr_ring_scan_poll (same protocol as
    // live_cursor() above, separate trigger state): the mid-scan fault advances the
    // producer BETWEEN the bracket's entry and exit cursor reads, i.e. while the
    // correlation is reading the live matrix.
    uint32_t scan_cursor() {
        ++scan_cursor_calls;
        if (fault_rows_mid_scan > 0 && scan_cursor_calls == fault_trigger_scan_cursor_call) {
            feed_noise_symbol_rows((size_t)fault_rows_mid_scan);
            fault_rows_mid_scan = 0;  // fire once
        }
        return wspr_portapack_total_br(&st);
    }

    // Feed n SYMBOL rows (2n block-rows) of unit Gaussian noise through the real
    // producer: every completed block-row is quantized and published exactly like
    // on-air capture, overwriting the ring slots it passes. (One symbol row is
    // exactly kSamplesPerSymbol 12 kHz samples = TWO OSR2 block-rows.)
    void feed_noise_symbol_rows(size_t symbol_rows) {
        Noise noise{777u + (uint32_t)st.total_br};
        const size_t samples = symbol_rows * (size_t)kSamplesPerSymbol;
        for (size_t i = 0; i < samples; i++) {
            wspr_portapack_feed_sample(&st, noise.gaussian());
        }
    }

    // The firmware decode thread's per-candidate steps, on ONE released candidate:
    // guarded ring snapshot -> soft weights -> slot-aligned accumulate -> report.
    void process_candidate(uint32_t br_now, uint32_t cand_br, int16_t cand_col, int16_t cand_excess, int8_t cand_drift) {
        wspr_snapshot_t snap;
        int snap_fail = WSPR_SNAP_BAD_ARGS;
        if (wspr_ring_snapshot_candidate(wspr_portapack_matrix(), wspr_portapack_bins(), WSPR_RX_BLOCK_ROWS, br_now, cand_br, cand_col, wspr_portapack_bin0_hz(), cand_drift, cand_excess, &snap, [](void* p) { return ((RingRx*)p)->live_cursor(); }, this, &snap_fail)) {
            ++snap_ok_count;
            // Exactly the firmware decode thread's steps: soft weights
            // from the snapshot (no matrix), then the slot-aligned
            // track accumulator. Only an integrated decode reports.
            int8_t sym_weight[WSPR_CODED_BITS];
            if (wspr_snapshot_demod_weights(&snap, sym_weight)) {
                wspr_decode_result_t integrated;
                if (wspr_agg_track_contribute(&snap, sym_weight, cand_br,
                                              &integrated)) {
                    // Dedup anchor only on a REPORTED decode, exactly as
                    // the firmware decode thread does: a failed attempt
                    // leaves the aligned bank's neighbouring start
                    // scannable.
                    wspr_ring_scan_mark_decoded(&scan, cand_br, cand_col);
                    last_excess = (uint32_t)cand_excess;
                    decodes.push_back(DecodeRecord{cand_br, br_now, cand_col, cand_excess, integrated});
                } else {
                    contribs.push_back(ContributionRecord{
                        cand_br, cand_col, snap.meta.drift, snap.meta.bank,
                        std::vector<int8_t>(sym_weight,
                                            sym_weight + WSPR_CODED_BITS),
                        wspr_agg_last_result_code(), cand_excess});
                }
            }
        } else {
            // The candidate was dropped (expired past the overwrite deadline
            // or raced the writer): count it like the firmware does and
            // remember the reason for the assertions.
            last_snap_fail = snap_fail;
            scan.snap_drops++;
        }
    }

    void feed(const std::vector<float>& samples) {
        for (const float s : samples) {
            // Worker asleep: capture keeps running on the real stream, no polling.
            if (worker_sleep_samples_left > 0) {
                worker_sleep_samples_left--;
                wspr_portapack_feed_sample(&st, s);
                if (worker_sleep_samples_left == 0 && deferred.valid) {
                    // The worker wakes holding the candidate it had released before
                    // the preemption; the snapshot runs NOW, against the cursor as it
                    // is after the sleep - the guarded reader's job to accept or drop.
                    process_candidate(wspr_portapack_total_br(&st), deferred.br,
                                      deferred.col, deferred.excess, deferred.drift);
                    deferred.valid = false;
                }
                continue;
            }
            if (!wspr_portapack_feed_sample(&st, s)) continue;

            uint32_t br = wspr_portapack_total_br(&st);
            uint32_t start_br = 0;
            int16_t col = 0;
            int16_t excess = 0;
            int8_t drift = 0;
            while (true) {
                // FRESH cursor per job, exactly like the firmware drain loop: a
                // latched total_br would let the deadline check pass starts the
                // producer already rewrote.
                br = wspr_portapack_total_br(&st);
                const auto t0 = std::chrono::steady_clock::now();
                const int have =
                    wspr_ring_scan_poll(wspr_portapack_matrix(), wspr_portapack_bins(), WSPR_RX_BLOCK_ROWS, br, &scan, WSPR_RING_SCAN_MAX_STARTS_PER_POLL, [](void* p) { return ((RingRx*)p)->scan_cursor(); }, this, &start_br, &col, &excess, &drift);
                const auto t1 = std::chrono::steady_clock::now();
                scan_ns += (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
                if (!have) {
                    // poll returns 0 BOTH when caught up AND when the per-call start
                    // budget is exhausted. The firmware drain loop re-polls while the
                    // producer has moved; mirror that here, or a budget exhaustion
                    // under a fault advance would stall the harness drain and expire
                    // starts the real decode thread would have consumed.
                    if ((int32_t)(br - scan.next_scan_br) < WSPR_CODED_BITS * 2 - 1) break;
                    continue;
                }
                scan_calls++;

                // Fault: the worker is preempted for k symbol-rows by the sequential
                // decode of a REAL candidate - the candidate's snapshot is deferred
                // until the sleep ends, and the producer runs on meanwhile.
                if (fault_rows_after_release > 0 && excess >= kFaultMinExcess) {
                    deferred = Deferred{start_br, col, excess, drift, true};
                    worker_sleep_samples_left +=
                        (uint64_t)fault_rows_after_release * kSamplesPerSymbol;
                    break;
                }

                process_candidate(br, start_br, col, excess, drift);
            }
        }
    }

    // Active track count.
    int active_tracks() const {
        wspr_agg_track_info_t info[WSPR_AGG_TRACKS];
        const int n = wspr_agg_tracks(info, WSPR_AGG_TRACKS);
        int active = 0;
        for (int i = 0; i < n; i++)
            if (info[i].active) active++;
        return active;
    }
    // The active track within +-2 columns of `col` (found = false if none).
    struct TrackMatch {
        bool found = false;
        wspr_agg_track_info_t info{};
    };
    TrackMatch track_at_col(int col) const {
        wspr_agg_track_info_t info[WSPR_AGG_TRACKS];
        const int n = wspr_agg_tracks(info, WSPR_AGG_TRACKS);
        for (int i = 0; i < n; i++) {
            if (info[i].active && info[i].col0 >= col - 2 && info[i].col0 <= col + 2) {
                return TrackMatch{true, info[i]};
            }
        }
        return TrackMatch{};
    }
};

constexpr const char* kCallsign = "K1ABC";
constexpr const char* kGrid = "FN20";

// Absolute block-row id where a TX starting at stream sample `start_sample` is expected
// to anchor: the first row boundary at or after its start, on the nearer bank.
uint32_t expected_start_br(uint64_t start_sample) {
    const uint64_t row = start_sample / kSamplesPerSymbol;  // symbol row of that grid
    // The packet's symbol 0 lands on whichever bank's row boundary is within half a
    // symbol; its absolute block-row id is 2*row or 2*row + 1. The scan must find one
    // of them.
    return (uint32_t)(2 * row);
}

// Feed ONE transmission slot plus a second one WSPR slot (120 s) later with an
// independent noise seed, the canonical two-slot integration scenario.
std::vector<float> two_slot_stream(const uint8_t* tones, uint64_t offset_s, float snr_db, uint32_t seed, int drift = 0) {
    const uint64_t offset = offset_s * WSPR_SAMPLE_RATE;
    const size_t len = (size_t)offset + (size_t)(2 * kSlotSamples) +
                       (WSPR_CODED_BITS + 8) * kSamplesPerSymbol;
    std::vector<float> samples;
    synthesize_stream(tones, offset, len, snr_db, seed, samples, drift);
    add_tx(samples, tones, offset + kSlotSamples, snr_db, seed + 1, drift);
    return samples;
}

}  // namespace

TEST_CASE("streaming: two slots of one -14 dB signal integrate into exactly one report") {
    uint8_t tones[WSPR_CODED_BITS];
    REQUIRE(wspr_encode_message(kCallsign, kGrid, 37, tones));

    for (const uint64_t offset_s : {0ull, 37ull, 77ull, 113ull}) {
        CAPTURE(offset_s);
        RingRx rx;
        rx.feed(two_slot_stream(tones, offset_s, -14.0f, 1000 + (uint32_t)offset_s));

        // Slot 1 alone never reports; slot 2 completes the integration and the
        // single integrated decode reports. Nothing else reports (no impostor
        // per-window decodes, no ghost re-decodes).
        REQUIRE_EQ(rx.decodes.size(), 1u);
        const DecodeRecord& d = rx.decodes[0];
        CHECK_EQ(std::string(d.result.callsign), std::string(kCallsign));
        CHECK_EQ(std::string(d.result.grid), std::string(kGrid));
        CHECK_EQ(d.result.dbm, 37);
        // Integration depth of the report: exactly two accumulated slots.
        CHECK_EQ((int)wspr_agg_last_decode_slots(), 2);
        // The track reset on success: no active track remains.
        CHECK_EQ(rx.active_tracks(), 0);
        // The reported candidate anchors within two block-rows (one symbol row per
        // bank) of the SECOND slot's expected start.
        const uint32_t expect2 =
            expected_start_br(offset_s * WSPR_SAMPLE_RATE + kSlotSamples);
        CAPTURE(expect2);
        CAPTURE(d.start_br);
        CHECK(d.start_br + 2u >= expect2);
        CHECK(d.start_br <= expect2 + 2u);
        // Decoded within one slot-span of the reported packet's start.
        CHECK(d.decoded_at_br - d.start_br <= 2u * (WSPR_CODED_BITS + 2));
        // The candidate must pass the incremental-scan gate.
        CHECK(rx.last_excess >= WSPR_RING_MIN_EXCESS);
        // Exactly one scan was owed per completed block-row at the end (drained).
        CHECK_EQ(rx.scan.next_scan_br, wspr_portapack_total_br(&rx.st) - 322u);
    }
}

TEST_CASE("streaming: decode at arbitrary capture phases, -20 dB, two slots") {
    uint8_t tones[WSPR_CODED_BITS];
    REQUIRE(wspr_encode_message(kCallsign, kGrid, 37, tones));

    for (const uint64_t offset_s : {0ull, 37ull, 77ull, 113ull}) {
        CAPTURE(offset_s);
        RingRx rx;
        rx.feed(two_slot_stream(tones, offset_s, -20.0f, 2000 + (uint32_t)offset_s));

        REQUIRE_EQ(rx.decodes.size(), 1u);
        const DecodeRecord& d = rx.decodes[0];
        CHECK_EQ(std::string(d.result.callsign), std::string(kCallsign));
        CHECK_EQ(std::string(d.result.grid), std::string(kGrid));
        CHECK_EQ((int)wspr_agg_last_decode_slots(), 2);
        const uint32_t expect2 =
            expected_start_br(offset_s * WSPR_SAMPLE_RATE + kSlotSamples);
        CHECK(d.start_br + 2u >= expect2);
        CHECK(d.start_br <= expect2 + 2u);
        CHECK(d.decoded_at_br - d.start_br <= 2u * (WSPR_CODED_BITS + 2));
    }
}

TEST_CASE("streaming: single slot never reports but holds count = 1") {
    uint8_t tones[WSPR_CODED_BITS];
    REQUIRE(wspr_encode_message(kCallsign, kGrid, 37, tones));

    // One full transmission plus a generous noise tail (the ghost/dedup region a
    // single slot leaves scannable).
    const size_t len = (size_t)(WSPR_CODED_BITS + 40) * kSamplesPerSymbol;
    std::vector<float> samples;
    synthesize_stream(tones, 0, len, -14.0f, 3000, samples);

    RingRx rx;
    rx.feed(samples);

    // No report at count = 1: the single-slot per-window decode (unreliable,
    // impostor-prone) is exactly what the accumulator replaced.
    REQUIRE_EQ(rx.decodes.size(), 0u);
    // The slot's weights ARE held: one active track, one accumulated slot.
    const auto tm = rx.track_at_col(10);
    REQUIRE(tm.found);
    CHECK_EQ((int)tm.info.count, 1);

    // A second slot one WSPR slot later then reports: the count reaches the
    // integration gate. Same stream continued, independent noise seed.
    const size_t total = (size_t)kSlotSamples + (WSPR_CODED_BITS + 8) * kSamplesPerSymbol;
    std::vector<float> two(len < total ? total : len, 0.0f);
    for (size_t i = 0; i < len; i++) two[i] = samples[i];
    add_tx(two, tones, kSlotSamples, -14.0f, 3001);

    RingRx rx2;
    rx2.feed(two);
    REQUIRE_EQ(rx2.decodes.size(), 1u);
    CHECK_EQ(std::string(rx2.decodes[0].result.callsign), std::string(kCallsign));
    CHECK_EQ((int)wspr_agg_last_decode_slots(), 2);
    CHECK_EQ(rx2.active_tracks(), 0);
}

TEST_CASE("streaming: two same-start stations 100 columns apart integrate independently") {
    uint8_t tones_a[WSPR_CODED_BITS];
    REQUIRE(wspr_encode_message(kCallsign, kGrid, 37, tones_a));
    uint8_t tones_b[WSPR_CODED_BITS];
    REQUIRE(wspr_encode_message("W9XYZ", "EN37", 27, tones_b));

    // Both stations start together; their tone-0 columns are 10 and 110 (absolute
    // bins 966 and 1066), far beyond the +-2 column-match tolerance. Each gets
    // two slots; each station must produce exactly one integrated report with
    // its own payload - no cross-talk between the two tracks.
    const size_t len = (size_t)(2 * kSlotSamples) + (WSPR_CODED_BITS + 8) * kSamplesPerSymbol;
    std::vector<float> samples;
    synthesize_stream(tones_a, 0, len, -14.0f, 5000, samples);
    add_tx(samples, tones_a, kSlotSamples, -14.0f, 5001);
    add_tx_bin(samples, tones_b, kSynthBaseBin + 100, 0, -14.0f, 5002);
    add_tx_bin(samples, tones_b, kSynthBaseBin + 100, kSlotSamples, -14.0f, 5003);

    RingRx rx;
    rx.feed(samples);

    REQUIRE_EQ(rx.decodes.size(), 2u);
    bool got_a = false, got_b = false;
    for (const DecodeRecord& d : rx.decodes) {
        if (std::string(d.result.callsign) == kCallsign) {
            got_a = true;
            CHECK_EQ(std::string(d.result.grid), std::string(kGrid));
            CHECK_EQ(d.result.dbm, 37);
        } else if (std::string(d.result.callsign) == "W9XYZ") {
            got_b = true;
            CHECK_EQ(std::string(d.result.grid), std::string("EN37"));
            CHECK_EQ(d.result.dbm, 27);
        }
    }
    REQUIRE(got_a);
    REQUIRE(got_b);
    CHECK_EQ((int)wspr_agg_last_decode_slots(), 2);
    // Both tracks reset on their successful decodes.
    CHECK_EQ(rx.active_tracks(), 0);
}

TEST_CASE("streaming: message change after a decode starts a fresh cycle") {
    // Slots 1 and 2 carry dbm 37; the integrated decode reports and RESETS the
    // track. Slot 3 carries a different message (dbm 27): it must not produce a
    // second report of the old message, and it sits at count = 1 at the end.
    uint8_t tones_old[WSPR_CODED_BITS];
    REQUIRE(wspr_encode_message(kCallsign, kGrid, 37, tones_old));
    uint8_t tones_new[WSPR_CODED_BITS];
    REQUIRE(wspr_encode_message(kCallsign, kGrid, 27, tones_new));

    const size_t len = (size_t)(3 * kSlotSamples) + (WSPR_CODED_BITS + 8) * kSamplesPerSymbol;
    std::vector<float> samples;
    synthesize_stream(tones_old, 0, len, -14.0f, 6000, samples);
    add_tx(samples, tones_old, kSlotSamples, -14.0f, 6001);
    add_tx(samples, tones_new, 2 * kSlotSamples, -14.0f, 6002);

    RingRx rx;
    rx.feed(samples);

    // Exactly one report: the integrated decode of slots 1+2.
    REQUIRE_EQ(rx.decodes.size(), 1u);
    CHECK_EQ(std::string(rx.decodes[0].result.callsign), std::string(kCallsign));
    CHECK_EQ(rx.decodes[0].result.dbm, 37);
    CHECK_EQ((int)wspr_agg_last_decode_slots(), 2);
    // Slot 3 opened a fresh track at count = 1: no second report of either
    // message within the feed.
    CHECK_EQ(rx.active_tracks(), 1);
    const auto tm = rx.track_at_col(10);
    REQUIRE(tm.found);
    CHECK_EQ((int)tm.info.count, 1);
}

TEST_CASE("streaming: decodes a transmission with nonzero drift, two slots") {
    uint8_t tones[WSPR_CODED_BITS];
    REQUIRE(wspr_encode_message(kCallsign, kGrid, 37, tones));

    for (const int drift : {-3, -2, 2, 3}) {
        CAPTURE(drift);
        RingRx rx;
        // Launch mid-band: the TX starts at stream sample 30 s. Both slots carry
        // the same drift; the integrated report carries the newest slot's
        // refined drift.
        rx.feed(two_slot_stream(tones, 30, -14.0f, 2000u + (uint32_t)(drift + 3), drift));

        std::cerr << "drift=" << drift << " last_excess=" << rx.last_excess
                  << " scanned=" << rx.scan.scanned << " expired=" << rx.scan.expired
                  << " decodes=" << rx.decodes.size() << std::endl;
        REQUIRE_EQ(rx.decodes.size(), 1u);
        const DecodeRecord& d = rx.decodes[0];
        CHECK_EQ(std::string(d.result.callsign), std::string(kCallsign));
        CHECK_EQ(std::string(d.result.grid), std::string(kGrid));
        CAPTURE(d.result.drift_bins);
        CHECK_EQ((int)d.result.drift_bins, drift);
        CHECK(d.result.agreement_pct >= 80);
    }
}

TEST_CASE("streaming: uint32 counter wrap decodes across the seam") {
    uint8_t tones[WSPR_CODED_BITS];
    REQUIRE(wspr_encode_message(kCallsign, kGrid, 37, tones));

    RingRx rx;
    // Force the absolute counters near the uint32 wrap (2^32 - 100). 2^32 is not a
    // multiple of 352, so the counter wraps mid-session: ids continue below 100 while
    // the slot formula stays id % 352 on the raw wrapping ids.
    const uint32_t base = 0xFFFFFF9Cu;
    rx.st.total_br = base;
    rx.scan.next_scan_br = base;

    // Pre-roll of 102 symbol periods of noise, then TWO slots one WSPR slot apart:
    // the first integrates silently (count = 1, no report), the second reports.
    const uint64_t tx_start = 102ull * kSamplesPerSymbol;
    const size_t len = (size_t)tx_start + (size_t)kSlotSamples +
                       (WSPR_CODED_BITS + 8) * kSamplesPerSymbol;
    Noise pre{60606};
    std::vector<float> samples(len, 0.0f);
    for (size_t i = 0; i < (size_t)tx_start; i++) samples[i] = pre.gaussian();
    {
        std::vector<float> tx1;
        synthesize_stream(tones, 0, len - (size_t)tx_start, -14.0f, 1000, tx1);
        for (size_t i = (size_t)tx_start; i < len; i++) samples[i] += tx1[i - (size_t)tx_start];
    }
    add_tx(samples, tones, tx_start + kSlotSamples, -14.0f, 1001);
    rx.feed(samples);

    // Only the second slot reports, on a post-wrap id: the first slot's anchor sat
    // near 104 (204 block-rows past `base`, wrapped), the second near 104 + 352.
    REQUIRE_EQ(rx.decodes.size(), 1u);
    const DecodeRecord& d = rx.decodes[0];
    CHECK_EQ(std::string(d.result.callsign), std::string(kCallsign));
    CHECK_EQ((int)wspr_agg_last_decode_slots(), 2);
    CHECK(d.start_br >= 440u);
    CHECK(d.start_br <= 470u);
    CHECK(d.decoded_at_br < base);  // the counter wrapped between anchor and decode
    CHECK(d.decoded_at_br > d.start_br);
    CHECK_EQ(rx.scan.next_scan_br, wspr_portapack_total_br(&rx.st) - 322u);
}

TEST_CASE("streaming: 60 minutes of noise-only feed reports nothing, tracks clean") {
    RingRx rx;
    // 3600 s of cheap white noise at 12 kHz, fed in chunks: every block-row scans,
    // no candidate ever clears the excess gate, no track ever opens, no report
    // ever fires.
    const uint64_t total = 3600ull * WSPR_SAMPLE_RATE;
    FastNoise noise{424242u};
    std::vector<float> chunk(1u << 16);
    for (uint64_t fed = 0; fed < total; fed += chunk.size()) {
        for (size_t i = 0; i < chunk.size(); i++) chunk[i] = noise.next_sample();
        rx.feed(chunk);
    }

    CHECK_EQ(rx.decodes.size(), 0u);
    CHECK_EQ(rx.active_tracks(), 0);
    // The scan stayed live the whole hour: no wedge, no backlog.
    CHECK_EQ(rx.scan.next_scan_br, wspr_portapack_total_br(&rx.st) - 322u);
    std::cerr << "noise hour: scanned=" << rx.scan.scanned
              << " expired=" << rx.scan.expired << std::endl;
}

TEST_CASE("scan cost: per-start sync correlation wall time (host)") {
    wspr_portapack_state_t st;
    REQUIRE(wspr_portapack_init(&st));
    Noise noise{9000};
    // Fill the ring with noise: the scan cost on a quiet band is the budget case.
    for (uint32_t i = 0; i < (uint32_t)WSPR_RX_BLOCK_ROWS * kSamplesPerSymbol; i++) {
        wspr_portapack_feed_sample(&st, noise.gaussian());
    }
    wspr_ring_scan_state_t scan;
    wspr_ring_scan_init(&scan);

    const uint32_t br = wspr_portapack_total_br(&st);
    // Deadline bound: a start may sit at most 352 - 12 = 340 rows behind the cursor,
    // so reps stay within 323..340 rows back and each poll runs exactly one scan.
    const int reps = 15;
    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < reps; i++) {
        // Scan start hypotheses by shifting the session base one row per rep: each
        // poll runs exactly one full-column sync scan.
        wspr_ring_scan_state_t tmp = scan;
        tmp.next_scan_br = br - 323u - (uint32_t)i;
        uint32_t s = 0;
        int16_t col = 0, excess = 0;
        int8_t drift = 0;
        wspr_ring_scan_poll(wspr_portapack_matrix(), wspr_portapack_bins(),
                            WSPR_RX_BLOCK_ROWS, br, &tmp,
                            WSPR_RING_SCAN_MAX_STARTS_PER_POLL,
                            nullptr, nullptr,
                            &s, &col, &excess, &drift);
    }
    const auto t1 = std::chrono::steady_clock::now();
    const double us_per_start =
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count() / 1000.0 / reps;
    MESSAGE("host scan per start: ", us_per_start, " us");
#if defined(__SANITIZE_ADDRESS__) || (defined(__clang__) && __has_feature(address_sanitizer))
    // Sanitized build (ASAN+UBSAN, the only configuration this suite ships): the
    // sanitizers instrument every byte load of the 134x7x162 correlation and slow it
    // several-fold, so the tuned 20 ms unsanitized budget is not observable here and
    // the ARM budget must come from a non-sanitized firmware build (map/-fstack-usage),
    // not from this binary. What IS invariant and still checked: the scan must stay
    // REAL-TIME FEASIBLE on the host even instrumented - far below one block-row
    // period (352 block-rows = 120.15 s, so one row is ~341 ms).
    CHECK(us_per_start < 341000.0);
#else
    // Informational budget bound: the scan must stay far below one block-row period
    // (341 ms) even on the host; the 20 ms figure is the tuned unsanitized budget.
    CHECK(us_per_start < 20000.0);
#endif
}

// ---- Producer-preemption / fault injection ----
// The worker (scan/snapshot) is delayed while the producer keeps writing, exactly
// what a seconds-long FEC decode does to the decode thread on the M4. Delays are in
// SYMBOL rows (1 symbol row = 2 block-rows of the OSR2 ring).

TEST_CASE("fault injection: worker delayed 0/1/5 symbol-rows still decodes the right message") {
    uint8_t tones[WSPR_CODED_BITS];
    REQUIRE(wspr_encode_message(kCallsign, kGrid, 37, tones));

    for (const int delay_rows : {0, 1, 5}) {
        CAPTURE(delay_rows);
        RingRx rx;
        rx.fault_rows_after_release = delay_rows;
        rx.feed(two_slot_stream(tones, 30, -14.0f, 7000u + (uint32_t)delay_rows));

        // The delayed worker still caught candidates inside the overwrite deadline:
        // the integrated decode reports the exact payload, not garbage read from a
        // rewritten ring.
        REQUIRE_EQ(rx.decodes.size(), 1u);
        const DecodeRecord& d = rx.decodes[0];
        CHECK_EQ(std::string(d.result.callsign), std::string(kCallsign));
        CHECK_EQ(std::string(d.result.grid), std::string(kGrid));
        CHECK_EQ(d.result.dbm, 37);
        CHECK_EQ((int)wspr_agg_last_decode_slots(), 2);
        CHECK(rx.scan.snap_drops == 0u);
        CHECK(rx.last_snap_fail == WSPR_SNAP_OK);
        // Nothing was read past the deadline: the worst lag stayed inside the margin.
        CHECK(rx.scan.max_lag <= (uint32_t)WSPR_RX_BLOCK_ROWS - WSPR_RING_DEADLINE_ROWS);
    }
}

TEST_CASE("fault injection: 15 symbol-rows delay drops expired jobs with counted reasons") {
    uint8_t tones[WSPR_CODED_BITS];
    REQUIRE(wspr_encode_message(kCallsign, kGrid, 37, tones));

    RingRx rx;
    rx.fault_rows_after_release = 15;  // 30 block-rows: past the 340-row deadline
    rx.feed(two_slot_stream(tones, 30, -14.0f, 7100));

    // Every candidate released into the delay is EXPIRED, never decoded: no decode
    // may be built from rows the producer has already rewritten (the first slot never
    // contributes, so the second slot cannot integrate either).
    CHECK_EQ(rx.decodes.size(), 0u);
    CHECK(rx.scan.snap_drops > 0u);
    CHECK(rx.scan.expired > 0u);
    // The counters report how far behind the worker actually was. scan_lag tracks the
    // LAST processed start (which may be a fresh one after the drops), so the worst
    // case lives in max_lag.
    CHECK(rx.scan.max_lag > (uint32_t)WSPR_RX_BLOCK_ROWS - WSPR_RING_DEADLINE_ROWS);
    CHECK(rx.last_snap_fail == WSPR_SNAP_EXPIRED);
    std::cerr << "15-row delay: snap_drops=" << rx.scan.snap_drops
              << " expired=" << rx.scan.expired << " max_lag=" << rx.scan.max_lag << std::endl;
}

TEST_CASE("fault injection: producer advancing MID-snapshot is caught by the guarded copy") {
    uint8_t tones[WSPR_CODED_BITS];
    REQUIRE(wspr_encode_message(kCallsign, kGrid, 37, tones));

    RingRx rx;
    // Arm the writer "racing" the reader: on the 25th live-cursor read inside the
    // snapshot (a few guarded row copies in), the producer advances 15 symbol rows,
    // overwriting the packet's oldest rows under the reader.
    // Fire on the 5th cursor read (entry + ~2 guarded rows in): at entry lag ~327,
    // a jump of 25 symbol rows (50 block-rows) pushes the cursor past the overwrite
    // point of the packet's early rows (age > block_rows), which the per-row
    // post-copy guard must catch. The default 15-row jump only becomes unsafe past
    // symbol row ~12, so the trigger and the jump are matched here.
    rx.fault_rows_mid_snapshot = 25;
    rx.fault_trigger_cursor_call = 5;
    rx.feed(two_slot_stream(tones, 30, -14.0f, 7200));

    // The per-row pre/post guard must have detected the race: the snapshot was
    // rejected as OVERWRITTEN, counted, and no decode ran on torn bytes.
    CHECK_EQ(rx.decodes.size(), 0u);
    CHECK_EQ(rx.last_snap_fail, WSPR_SNAP_OVERWRITTEN);
    CHECK(rx.scan.snap_drops > 0u);
    std::cerr << "mid-snapshot race: snap_drops=" << rx.scan.snap_drops << std::endl;
}

// ---- Review point 2: producer advance/overwrite DURING the correlation itself
// (not merely between poll jobs or around a snapshot copy). The scan reads the
// LIVE matrix for its whole ~134x7x162 pass, so a preempting producer can rewrite
// the packet's rows mid-scan; the entry/exit live-cursor bracket must detect that
// and the whole scan result must be discarded (stale_scans) before it can become a
// pending/secondary candidate. ----

TEST_CASE("scan staleness bracket: scripted cursor catches a mid-correlation overwrite") {
    const int bins = wspr_portapack_bins();
    const uint32_t rows = (uint32_t)WSPR_RX_BLOCK_ROWS;
    std::vector<uint8_t> mag((size_t)rows * (size_t)bins);
    // Deterministic per-id pattern so any result computed over rewritten rows is
    // detectable by construction.
    for (uint32_t r = 0; r < rows; r++)
        for (int b = 0; b < bins; b++)
            mag[(size_t)r * (size_t)bins + (size_t)b] = (uint8_t)((r * 7u + (uint32_t)b) & 0xFFu);

    struct CursorCtx {
        uint32_t* cursor;
        int jump_at_call;  // 1-based; 0 disables
        uint32_t jump_to;
        int calls = 0;
    };
    auto cursor_fn = [](void* p) -> uint32_t {
        auto* c = (CursorCtx*)p;
        ++c->calls;
        if (c->jump_at_call > 0 && c->calls == c->jump_at_call) *c->cursor = c->jump_to;
        return *c->cursor;
    };

    int16_t cols[8];
    int16_t excesses[8];
    int8_t drifts[8];

    // (a) Quiescent producer (cursor never moves): the scan of a fully published
    // start completes and is NOT flagged stale.
    {
        uint32_t cursor = rows;
        CursorCtx ctx{&cursor, 0, 0};
        const uint32_t start = rows - 330u;
        const int found = wspr_ring_scan_start(mag.data(), bins, rows, cursor, start,
                                               WSPR_RING_MIN_EXCESS, 4,
                                               cursor_fn, &ctx, cols, excesses, drifts);
        CHECK(found >= 0);
        CHECK_EQ(ctx.calls, 2);  // entry + exit bracket reads, exactly
    }

    // (b) Mid-correlation overwrite: the cursor jumps past the packet's overwrite
    // point between the ENTRY and EXIT bracket reads - i.e. while the correlation
    // is running. The scan must refuse its own result with -1, whatever it found.
    {
        uint32_t cursor = rows;
        CursorCtx ctx{&cursor, 2, rows + 48};
        const uint32_t start = rows - 330u;
        const int found = wspr_ring_scan_start(mag.data(), bins, rows, cursor, start,
                                               WSPR_RING_MIN_EXCESS, 4,
                                               cursor_fn, &ctx, cols, excesses, drifts);
        CHECK_EQ(found, -1);
    }

    // (c) Already stale at entry: nothing is correlated at all, refused up front.
    {
        uint32_t cursor = rows + 48;
        CursorCtx ctx{&cursor, 0, 0};
        const uint32_t start = rows - 330u;
        const int found = wspr_ring_scan_start(mag.data(), bins, rows, cursor, start,
                                               WSPR_RING_MIN_EXCESS, 4,
                                               cursor_fn, &ctx, cols, excesses, drifts);
        CHECK_EQ(found, -1);
        CHECK_EQ(ctx.calls, 1);  // entry read only: no byte of the matrix touched
    }
}

TEST_CASE("scan staleness bracket: a stale scan is discarded by the poll, reason counted") {
    const int bins = wspr_portapack_bins();
    const uint32_t rows = (uint32_t)WSPR_RX_BLOCK_ROWS;
    std::vector<uint8_t> mag((size_t)rows * (size_t)bins);
    for (uint32_t r = 0; r < rows; r++)
        for (int b = 0; b < bins; b++)
            mag[(size_t)r * (size_t)bins + (size_t)b] = (uint8_t)((r * 7u + (uint32_t)b) & 0xFFu);

    // The producer advances past the packet's overwrite point while the FIRST
    // scanned start's correlation is running (jump between the bracket's entry and
    // exit reads). The poll must discard that scan (stale_scans), never emit it as
    // a pending/secondary candidate, and keep making progress on the remaining
    // starts (they expire against the moved cursor, counted).
    uint32_t cursor = rows;
    struct Ctx {
        uint32_t* cursor;
        int calls = 0;
    } ctx{&cursor};
    auto cursor_fn = [](void* p) -> uint32_t {
        auto* c = (Ctx*)p;
        ++c->calls;
        if (c->calls == 2) *c->cursor += 48;  // fires on the FIRST scan's exit read
        return *c->cursor;
    };

    wspr_ring_scan_state_t st;
    wspr_ring_scan_init(&st);
    st.next_scan_br = rows - 330u;

    uint32_t s = 0;
    int16_t col = 0, excess = 0;
    int8_t drift = 0;
    const int have = wspr_ring_scan_poll(mag.data(), bins, rows, cursor, &st, 1,
                                         cursor_fn, &ctx, &s, &col, &excess, &drift);
    // Nothing was handed up on this poll: the raced scan was discarded before it
    // could become a candidate, and every later start of the moved window is past
    // the release limit.
    CHECK_EQ(have, 0);
    CHECK(st.stale_scans >= 1u);
    CHECK_FALSE(st.pending_valid);
    bool sec_valid = false;
    for (int i = 0; i < WSPR_RING_MAX_SECONDARY; i++)
        sec_valid = sec_valid || st.secondary[i].valid;
    CHECK_FALSE(sec_valid);
    // Bounded progress held: the next start still advanced despite the discard.
    CHECK(st.next_scan_br > rows - 330u);
    std::cerr << "mid-scan race: stale_scans=" << st.stale_scans
              << " expired=" << st.expired << std::endl;
}

TEST_CASE("scan staleness bracket: end-to-end feed with a mid-correlation producer advance") {
    uint8_t tones[WSPR_CODED_BITS];
    REQUIRE(wspr_encode_message(kCallsign, kGrid, 37, tones));

    RingRx rx;
    // Arm the writer racing the READER inside the scan: on the exit bracket read of
    // the first scanned start, the producer advances 25 symbol rows (50 block-rows),
    // overwriting rows the correlation is still reading.
    rx.fault_rows_mid_scan = 25;
    rx.fault_trigger_scan_cursor_call = 2;
    rx.feed(two_slot_stream(tones, 30, -14.0f, 7300));

    // The raced scan was detected and discarded with a counted reason - it never
    // became a pending/secondary candidate. Whatever DID report afterwards came
    // from clean scans: a decode, if any, must be the true payload (a corrupted
    // scan is never the evidence behind it).
    CHECK(rx.scan.stale_scans >= 1u);
    for (const auto& d : rx.decodes) {
        CHECK_EQ(std::string(d.result.callsign), std::string(kCallsign));
        CHECK_EQ(std::string(d.result.grid), std::string(kGrid));
        CHECK_EQ(d.result.dbm, 37);
    }
    std::cerr << "mid-scan feed race: stale_scans=" << rx.scan.stale_scans
              << " decodes=" << rx.decodes.size()
              << " snap_drops=" << rx.scan.snap_drops << std::endl;
}

TEST_CASE("scan session: a held pending candidate expires instead of releasing stale rows") {
    RingRx rx;
    // Fill the ring with noise, then plant a pending candidate whose lag is already
    // past the overwrite deadline (as if the worker slept through it).
    for (int i = 0; i < (int)WSPR_RX_BLOCK_ROWS + 4; i++) rx.feed_noise_symbol_rows(1);
    const uint32_t total = wspr_portapack_total_br(&rx.st);
    rx.scan.pending_valid = true;
    rx.scan.pending_br = total - ((uint32_t)WSPR_RX_BLOCK_ROWS - WSPR_RING_DEADLINE_ROWS + 5);
    const uint32_t expired_before = rx.scan.expired;

    uint32_t s = 0;
    int16_t col = 0, excess = 0;
    int8_t drift = 0;
    const int have = wspr_ring_scan_poll(wspr_portapack_matrix(), wspr_portapack_bins(),
                                         WSPR_RX_BLOCK_ROWS,
                                         wspr_portapack_total_br(&rx.st), &rx.scan,
                                         WSPR_RING_SCAN_MAX_STARTS_PER_POLL,
                                         nullptr, nullptr,
                                         &s, &col, &excess, &drift);
    // Expired BEFORE release: nothing handed up, counter reports it. (The budgeted
    // loop then goes on to expire up to max_starts already-dead starts as well.)
    CHECK_EQ(have, 0);
    CHECK_FALSE(rx.scan.pending_valid);
    CHECK(rx.scan.expired >= expired_before + 1u);
}

TEST_CASE("bounded scan progress: max_starts caps one poll; the backlog still drains") {
    RingRx rx;
    // A full ring of noise, then pretend the worker slept through most of it.
    for (int i = 0; i < (int)WSPR_RX_BLOCK_ROWS + 20; i++) rx.feed_noise_symbol_rows(1);
    const uint32_t total = wspr_portapack_total_br(&rx.st);
    rx.scan.next_scan_br = total - ((uint32_t)WSPR_RX_BLOCK_ROWS - 4);
    const uint32_t catch_up = total - (WSPR_CODED_BITS * 2 - 1);
    REQUIRE(rx.scan.next_scan_br < catch_up);

    uint32_t polls = 0;
    uint32_t prev_next = rx.scan.next_scan_br;
    // Poll returns 0 BOTH on caught-up AND on budget exhaustion; keep polling while
    // starts remain, which is exactly what the firmware drain loop does.
    while (rx.scan.next_scan_br < catch_up) {
        uint32_t s = 0;
        int16_t col = 0, excess = 0;
        int8_t drift = 0;
        wspr_ring_scan_poll(wspr_portapack_matrix(), wspr_portapack_bins(),
                            WSPR_RX_BLOCK_ROWS, wspr_portapack_total_br(&rx.st),
                            &rx.scan, 1, nullptr, nullptr,
                            &s, &col, &excess, &drift);
        ++polls;
        // BOUNDED WORK: one call consumes at most the one start it was allowed.
        CHECK(rx.scan.next_scan_br - prev_next <= 1u);
        prev_next = rx.scan.next_scan_br;
        REQUIRE(polls < 1000u);  // must terminate
    }
    // PROGRESS: despite the per-call cap, the scan caught up completely.
    CHECK_EQ(rx.scan.next_scan_br, catch_up);
    std::cerr << "backlog drain: polls=" << polls << " expired=" << rx.scan.expired << std::endl;
}

TEST_CASE("ring snapshot guard: scripted cursor catches a mid-copy overwrite") {
    const int bins = wspr_portapack_bins();
    const uint32_t rows = (uint32_t)WSPR_RX_BLOCK_ROWS;
    std::vector<uint8_t> mag((size_t)rows * (size_t)bins);
    // Deterministic per-id pattern in every slot: byte of row id r at column b is
    // (r*7 + b) & 0xFF, so a snapshot that read the WRONG (rewritten) row is
    // detectable byte-for-byte. The packet's own 4-tone block (col0 = 10, drift 0)
    // is then overwritten with a sync-friendly pattern (selected parity bins 200,
    // opposite/background 40) so the recomputed-from-frozen-tones belt excess
    // clears WSPR_RING_MIN_EXCESS: this test exercises the GUARD, not the gate.
    const uint32_t kStart = rows - 330u;
    for (uint32_t r = 0; r < rows; r++)
        for (int b = 0; b < bins; b++)
            mag[(size_t)r * (size_t)bins + (size_t)b] = (uint8_t)((r * 7u + (uint32_t)b) & 0xFFu);
    for (uint32_t n = 0; n < WSPR_CODED_BITS; n++) {
        const uint32_t r = kStart + 2u * n;  // stays < rows (kStart + 322 = rows - 8)
        const int g = (int)wspr_sync_vector[n];
        uint8_t* row = &mag[(size_t)(r % rows) * (size_t)bins];
        for (int i = 0; i < 4; i++) row[10 + i] = (uint8_t)((i == g || i == g + 2) ? 200 : 40);
    }

    struct CursorCtx {
        uint32_t* cursor;
        int jump_at_call;  // 1-based; 0 disables
        uint32_t jump_to;
        int calls = 0;
    };
    auto cursor_fn = [](void* p) -> uint32_t {
        auto* c = (CursorCtx*)p;
        ++c->calls;
        if (c->jump_at_call > 0 && c->calls == c->jump_at_call) *c->cursor = c->jump_to;
        return *c->cursor;
    };

    wspr_snapshot_t snap;
    int fail = WSPR_SNAP_BAD_ARGS;

    // (a) Quiescent producer, cursor = rows: the packet 330 rows back is fully
    // published and inside the deadline. The frozen bytes must match the planted
    // tone pattern EXACTLY (the guarded copy is correct, not merely non-crashing),
    // and the belt recomputation must record the EXCESS scale, not the raw score.
    {
        uint32_t cursor = rows;
        CursorCtx ctx{&cursor, 0, 0};
        const uint32_t start = kStart;
        const int ok = wspr_ring_snapshot_candidate(mag.data(), bins, rows, cursor, start,
                                                    10, 1400, 0, 20, &snap,
                                                    cursor_fn, &ctx, &fail);
        REQUIRE(ok);
        CHECK_EQ(fail, WSPR_SNAP_OK);
        for (int n = 0; n < WSPR_CODED_BITS; n++) {
            const int g = (int)wspr_sync_vector[n];
            for (int i = 0; i < 4; i++) {
                CAPTURE(n);
                CAPTURE(i);
                CHECK_EQ((int)snap.tones[n][i], (i == g || i == g + 2) ? 200 : 40);
            }
        }
        // Selected bins 200, opposite 40 -> excess = 200 - 40 = 160 on the
        // scan-excess scale (NOT the ~29000 raw score scale).
        CHECK(snap.meta.sync_score >= WSPR_RING_MIN_EXCESS);
        CHECK(snap.meta.sync_score <= 200);
        CHECK_EQ((int)snap.meta.sync_score, 160);
        CHECK_EQ(snap.meta.start_row_abs, start);
    }

    // (b) Expired: the cursor has moved within the deadline margin of the packet's
    // oldest row. Rejected with the EXPIRED reason, no bytes touched.
    {
        uint32_t cursor = rows + 30;  // lag = 330 + 30 > 352 - 12
        CursorCtx ctx{&cursor, 0, 0};
        const uint32_t start = rows - 330u;
        fail = WSPR_SNAP_OK;
        CHECK_FALSE(wspr_ring_snapshot_candidate(mag.data(), bins, rows, cursor, start,
                                                 10, 1400, 0, 20, &snap,
                                                 cursor_fn, &ctx, &fail));
        CHECK_EQ(fail, WSPR_SNAP_EXPIRED);
    }

    // (c) Not yet published: cursor below start + 323. Rejected, never reads.
    {
        uint32_t cursor = rows - 330u + 322;  // last symbol row not published yet
        CursorCtx ctx{&cursor, 0, 0};
        const uint32_t start = rows - 330u;
        CHECK_FALSE(wspr_ring_snapshot_candidate(mag.data(), bins, rows, cursor, start,
                                                 10, 1400, 0, 20, &snap,
                                                 cursor_fn, &ctx, &fail));
    }

    // (d) Mid-copy overwrite: the cursor jumps PAST the overwrite point of the
    // packet's early rows while the guarded copies are running. The post-copy guard
    // must reject with OVERWRITTEN - the function proves it never handed up bytes
    // read from rewritten rows.
    {
        uint32_t cursor = rows;
        // Entry check: call 1 (lag 330, passes). The jump fires on the PRE-copy
        // cursor load of symbol row ~4 (call 10): cursor 400 means row 30's slot was
        // rewritten (row 382 published). Guard must fail on that row.
        CursorCtx ctx{&cursor, 10, rows + 48};
        const uint32_t start = rows - 330u;
        fail = WSPR_SNAP_OK;
        CHECK_FALSE(wspr_ring_snapshot_candidate(mag.data(), bins, rows, cursor, start,
                                                 10, 1400, 0, 20, &snap,
                                                 cursor_fn, &ctx, &fail));
        CHECK_EQ(fail, WSPR_SNAP_OVERWRITTEN);
    }
}

// ---- Belt gate regression: meta.sync_score must be on the SCAN-EXCESS scale
// (score / WSPR_CODED_BITS - mean, contract in wspr_decode.h), and frozen bytes
// whose recomputed correlation is weak must be rejected with WEAK_SCORE instead
// of feeding the accumulator's strong-grade logic. An earlier revision recorded
// the RAW score (~28000), which made the gate a no-op and let junk candidates
// pass the >= WSPR_AGG_OPEN_EXCESS_MIN threshold. ----
TEST_CASE("snapshot belt gate: sync_score on the excess scale; junk frozen data is WEAK_SCORE") {
    const int bins = wspr_portapack_bins();
    const uint32_t rows = (uint32_t)WSPR_RX_BLOCK_ROWS;
    const uint32_t start = rows - 330u;
    wspr_snapshot_t snap;
    int fail = WSPR_SNAP_BAD_ARGS;

    // (1) Realistic strong frozen packet: background 40 everywhere, the packet's
    // selected tone bins (col0 = 10, drift 0) at 90 -> balanced excess 50, i.e.
    // inside the scan-excess range a real -14 dB packet produces (5..60).
    {
        std::vector<uint8_t> mag((size_t)rows * (size_t)bins, 40);
        for (uint32_t n = 0; n < WSPR_CODED_BITS; n++) {
            const uint32_t r = start + 2u * n;
            const int g = (int)wspr_sync_vector[n];
            uint8_t* row = &mag[(size_t)(r % rows) * (size_t)bins];
            for (int i = 0; i < 4; i++) row[10 + i] = (uint8_t)((i == g || i == g + 2) ? 90 : 40);
        }
        const int ok = wspr_ring_snapshot_candidate(mag.data(), bins, rows, rows, start,
                                                    10, 1400, 0, 20, &snap,
                                                    nullptr, nullptr, &fail);
        REQUIRE(ok);
        CHECK_EQ(fail, WSPR_SNAP_OK);
        CHECK_EQ((int)snap.meta.sync_score, 50);
        CHECK(snap.meta.sync_score >= 5);
        CHECK(snap.meta.sync_score <= 60);
    }

    // (2) Junk frozen data (uniform bins, zero correlation): even with a strong
    // CLAIMED scan excess the snapshot must refuse with WEAK_SCORE - such bytes
    // must never reach the weights.
    {
        std::vector<uint8_t> mag((size_t)rows * (size_t)bins, 128);
        fail = WSPR_SNAP_OK;
        const int ok = wspr_ring_snapshot_candidate(mag.data(), bins, rows, rows, start,
                                                    10, 1400, 0, 20, &snap,
                                                    nullptr, nullptr, &fail);
        CHECK_FALSE(ok);
        CHECK_EQ(fail, WSPR_SNAP_WEAK_SCORE);
    }
}

// ---- DIAGNOSTIC: dump every released candidate's geometry and the reason
// the accumulator gave it (see wspr_agg_last_result_code), plus the
// correlation distributions used to calibrate the message-consistency
// threshold (see WSPR_AGG_MSG_CORR_NUM in wspr_decode.c). ----
namespace {
double corr_of(const std::vector<int8_t>& a, const std::vector<int8_t>& b) {
    double ma = 0, mb = 0;
    for (size_t i = 0; i < a.size(); i++) {
        ma += a[i];
        mb += b[i];
    }
    ma /= a.size();
    mb /= b.size();
    double num = 0, da = 0, db = 0;
    for (size_t i = 0; i < a.size(); i++) {
        double x = a[i] - ma, y = b[i] - mb;
        num += x * y;
        da += x * x;
        db += y * y;
    }
    return (da > 0 && db > 0) ? num / std::sqrt(da * db) : 1.0;
}
}  // namespace

TEST_CASE("DIAG: contribution trace and consistency-threshold calibration") {
    uint8_t tones[WSPR_CODED_BITS];
    REQUIRE(wspr_encode_message(kCallsign, kGrid, 37, tones));
    double rr_min = 9, rr_max = -9, jr_min = 9, jr_max = -9, jj_min = 9, jj_max = -9;
    int n_rr = 0, n_jr = 0, n_jj = 0;
    for (const uint64_t offset_s : {0ull, 37ull, 77ull, 113ull}) {
        CAPTURE(offset_s);
        RingRx rx;
        rx.feed(two_slot_stream(tones, offset_s, -14.0f, 1000 + (uint32_t)offset_s));
        std::cerr << "=== offset " << offset_s << ": decodes=" << rx.decodes.size()
                  << " contribs=" << rx.contribs.size() << " active_tracks="
                  << rx.active_tracks() << " snap_ok=" << rx.snap_ok_count << std::endl;
        for (size_t i = 0; i < rx.contribs.size(); i++) {
            const auto& c = rx.contribs[i];
            std::cerr << "  contrib[" << i << "] br=" << c.start_br << " col=" << c.col
                      << " drift=" << (int)c.drift << " bank=" << (int)c.bank
                      << " excess=" << c.excess << " reason=" << c.reason
                      << std::endl;
            for (size_t j = 0; j < i; j++) {
                const auto& p = rx.contribs[j];
                const double cr = corr_of(p.weights, c.weights);
                const bool p_real = p.excess >= 20, c_real = c.excess >= 20;
                if (p_real && c_real) {
                    n_rr++;
                    if (cr < rr_min) rr_min = cr;
                    if (cr > rr_max) rr_max = cr;
                } else if (p_real != c_real) {
                    n_jr++;
                    if (cr < jr_min) jr_min = cr;
                    if (cr > jr_max) jr_max = cr;
                } else {
                    n_jj++;
                    if (cr < jj_min) jj_min = cr;
                    if (cr > jj_max) jj_max = cr;
                }
            }
        }
    }
    std::cerr << "FPR study: real-real n=" << n_rr << " range=[" << rr_min << "," << rr_max
              << "] junk-real n=" << n_jr << " range=[" << jr_min << "," << jr_max
              << "] junk-junk n=" << n_jj << " range=[" << jj_min << "," << jj_max << "]"
              << std::endl;
}

TEST_CASE("DIAG: two-station and fault-delay release traces") {
    uint8_t tones_a[WSPR_CODED_BITS];
    REQUIRE(wspr_encode_message(kCallsign, kGrid, 37, tones_a));
    uint8_t tones_b[WSPR_CODED_BITS];
    REQUIRE(wspr_encode_message("W9XYZ", "EN37", 27, tones_b));
    {
        const size_t len = (size_t)(2 * kSlotSamples) + (WSPR_CODED_BITS + 8) * kSamplesPerSymbol;
        std::vector<float> samples;
        synthesize_stream(tones_a, 0, len, -14.0f, 5000, samples);
        add_tx(samples, tones_a, kSlotSamples, -14.0f, 5001);
        add_tx_bin(samples, tones_b, kSynthBaseBin + 100, 0, -14.0f, 5002);
        add_tx_bin(samples, tones_b, kSynthBaseBin + 100, kSlotSamples, -14.0f, 5003);
        RingRx rx;
        rx.feed(samples);
        std::cerr << "=== two stations: decodes=" << rx.decodes.size()
                  << " active=" << rx.active_tracks() << std::endl;
        for (const auto& d : rx.decodes)
            std::cerr << "  DECODE br=" << d.start_br << " col=" << d.col
                      << " call=" << d.result.callsign << " excess=" << d.excess << std::endl;
        for (const auto& c : rx.contribs)
            std::cerr << "  c br=" << c.start_br << " col=" << c.col
                      << " excess=" << c.excess << " reason=" << c.reason << std::endl;
    }
    for (const int delay_rows : {0, 1, 5}) {
        RingRx rx;
        rx.fault_rows_after_release = delay_rows;
        rx.feed(two_slot_stream(tones_a, 30, -14.0f, 7000u + (uint32_t)delay_rows));
        std::cerr << "=== fault " << delay_rows << ": decodes=" << rx.decodes.size()
                  << " snap_ok=" << rx.snap_ok_count << " drops=" << rx.scan.snap_drops
                  << " expired=" << rx.scan.expired
                  << " last_fail=" << rx.last_snap_fail << std::endl;
        for (const auto& c : rx.contribs)
            std::cerr << "  c br=" << c.start_br << " col=" << c.col
                      << " excess=" << c.excess << " reason=" << c.reason << std::endl;
    }
}
