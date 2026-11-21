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

#include "proc_wspr_rx.hpp"
#include "portapack_shared_memory.hpp"
#include "audio_dma.hpp"
#include "dsp_fir_taps.hpp"
#include "dsp_iir_config.hpp"
#include "event_m4.hpp"

#include <cstring>

// The ring scan/snapshot geometry (symbol n at start + 2n, one start per block-row,
// ring depth 352) exists only for the OSR2 producer; an OSR1 build must not compile
// the decode loop against it (the wspr_lib ring functions also reject such a ring at
// runtime - see WSPR_RING_MIN_BLOCK_ROWS).
static_assert(WSPR_RX_TIME_OSR == 2, "ring capture is OSR2-only");

// Dedicated decoder stack, matching the 4 KiB budget used by FT8 RX.
// Keep decoding separate from the small main/event-loop stack; DMA and RSSI
// processing also run on their own thread stacks.
WORKING_AREA(wspr_decode_thread_wa, 4096);

Thread* WSPRDecodeThread::thread_ = nullptr;

// Live producer cursor for the ring reader protocol (wspr_ring_cursor_fn): every
// scan job and every snapshot reads the counter FRESH through this accessor instead
// of trusting a value latched before a long FEC decode. The volatile load plus the
// compiler barrier inside wspr_portapack_total_br is the strongest primitive this
// codebase uses - a single-core M4 ordering argument (context switches preserve
// program order; the compiler cannot reorder bytes across the barrier), NOT a C11
// data-race-freedom claim. See the protocol note in wspr_decode.h.
static uint32_t wspr_ring_live_cursor(void* ctx) {
    return wspr_portapack_total_br(reinterpret_cast<wspr_portapack_state_t*>(ctx));
}

// Saturate an int16 into the byte-sized debug message slots.
static inline int8_t saturate_i8(const int16_t value) {
    if (value > 127) return 127;
    if (value < -128) return -128;
    return (int8_t)value;
}

#if WSPR_CANDIDATE_TRACE
// The ADMITTED candidate path stamps its FINAL disposition (freq/snap/agg) onto
// the WSPRDebugMessage it already pushes; the REFUSED candidate path (snapshot
// failure) is covered in AGGREGATE by the win line's snapE/snapO counters (M4
// diag_snap_expired/overwritten) — per-frequency detail for refused candidates
// was dropped to stay within the 40960 combined budget. A second queue-message
// type would cost ~96 B of M4 flash for its push template alone.
#endif

WSPRDecodeThread::WSPRDecodeThread(wspr_portapack_state_t* state, WSPRRxProcessor* processor)
    : state_{state}, processor_{processor} {
    wspr_ring_scan_init(&scan_);
    chBSemInit(&sem_, TRUE);  // start taken
    start();
}

WSPRDecodeThread::~WSPRDecodeThread() {
    if (thread_) {
        terminate_.store(true, std::memory_order_release);
        chBSemSignal(&sem_);
        chThdWait(thread_);
        thread_ = nullptr;
    }
}

void WSPRDecodeThread::start() {
    if (!thread_) {
        thread_ = chThdCreateStatic(
            wspr_decode_thread_wa, sizeof(wspr_decode_thread_wa),
            NORMALPRIO - 10,  // Lower than baseband (NORMALPRIO) so DMA processing wins.
            ThreadBase::fn, this);
    }
}

void WSPRDecodeThread::signal_row() {
    // The signal is a WAKEUP, not a per-row token: the loop drains everything the ring
    // scan owes (wspr_ring_scan_poll catches up to total_br) on every wake. Coalescing
    // keeps the semaphore count at 0/1, so a long decode cannot bank hundreds of stale
    // signals.
    bool expected = false;
    if (wake_pending_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        chBSemSignal(&sem_);
    }
}

void WSPRDecodeThread::run() {
    // The last known sync score rides on every status/debug message; a quiet band keeps
    // the previous value instead of flickering to zero.
    int16_t last_sync_score = 0;
    uint8_t analysis_index = 0;

    // Periodic status heartbeat (no candidate needed). Ticks are milliseconds on
    // this baseband build; MS2ST keeps it correct if that ever changes.
    constexpr systime_t HEARTBEAT_INTERVAL = MS2ST(10000);  // ~1/10 s
    systime_t last_heartbeat = chTimeNow();

    while (true) {
        chBSemWait(&sem_);
        if (terminate_.load(std::memory_order_acquire)) break;

        // RX RESTART: reset what THIS thread owns (ring scan session + slot
        // accumulator + its diag counters). The producer-owned decoder_state
        // and the matrix were reset by the processor on the message thread
        // before this flag was set - each thread touches only its own state,
        // so no locks are involved. Any candidate job computed across the
        // boundary reads zeroed rows and is dropped by the excess/correlation
        // gates; memory safety does not depend on that (fixed arrays).
        if (restart_request_.exchange(false, std::memory_order_acq_rel)) {
            wspr_ring_scan_init(&scan_);
            wspr_decode_reset_aggregation();
            last_sync_score = 0;
            processor_->diag_decode_ms_valid.store(false, std::memory_order_relaxed);
            processor_->diag_probe_row.store(-1, std::memory_order_relaxed);
            processor_->diag_probe_br.store(0, std::memory_order_relaxed);
            // Session reset of THIS thread's observational counters.
            diag_cand_released = 0;
            diag_stage_entries = 0;
            diag_snap_expired = 0;
            diag_snap_overwritten = 0;
            diag_agg_reject = 0;
            diag_agg_fail = 0;
            diag_agg_other = 0;
            obs_freq_hz = 0;
            cand_pending = false;
        }

        busy_.store(true, std::memory_order_release);

        // Drain until the scan is caught up with the published ring. The cursor is
        // RE-READ before every candidate: a FEC decode can run for seconds, and a
        // stale total_br would let the scan's deadline check pass starts the producer
        // has already rewritten (and hand the snapshot those rows). Reading the
        // counter fresh per iteration keeps both gates honest; rows completed while
        // we worked are picked up either by this drain or by the next signal.
        uint32_t total_br = wspr_portapack_total_br(state_);
        do {
#if WSPR_RX_DIAG
            const systime_t decode_t0 = chTimeNow();
#endif
            uint32_t start_br = 0;
            int16_t col = 0;
            int16_t excess = 0;
            int8_t drift = 0;
            while (true) {
                // FRESH cursor per scan job: a stale total_br could pass starts the
                // producer has already rewritten.
                total_br = wspr_portapack_total_br(state_);
                if (!wspr_ring_scan_poll(wspr_portapack_matrix(),
                                         wspr_portapack_bins(),
                                         WSPR_RX_BLOCK_ROWS,
                                         total_br, &scan_,
                                         WSPR_RING_SCAN_MAX_STARTS_PER_POLL,
                                         wspr_ring_live_cursor, state_,
                                         &start_br, &col, &excess, &drift))
                    break;
                // Observational only: a candidate was RELEASED by the scan
                // (this is the single site all four release paths funnel
                // through). Frequency uses the same conversion as the decode
                // result (wspr_decode.c): bin0_hz + col * 12000 / 8192 Hz.
                // (v3: excess rides the existing probe channel; no drift.)
                obs_freq_hz = (uint16_t)(int16_t)(wspr_portapack_bin0_hz() + (col * WSPR_SAMPLE_RATE) / WSPR_SAMPLES_PER_SYMBOL);
                cand_pending = true;
                // Honest candidate count: this funnel is the ONE place every
                // released candidate passes (all four scan release paths
                // return through wspr_ring_scan_poll here).
                diag_cand_released++;
                // Per-candidate FINAL-disposition field (declared unconditionally
                // so the debug message always stamps it; only read by the M0 under
                // WSPR_CANDIDATE_TRACE). agg_code defaults to "never contributed"
                // (255) and is filled in after demod + aggregate contribute.
                uint8_t rec_agg_code = 0xFF;
#if WSPR_RX_DIAG
                // DECODE phase around every candidate attempt (snapshot + sequential
                // decode); capture continues underneath the whole time.
                processor_->publish_telemetry(WSPRTelemetryMessage::Phase::Decode);
                processor_->diag_probe_row.store((int16_t)start_br, std::memory_order_relaxed);
                processor_->diag_probe_score.store(excess, std::memory_order_relaxed);
                // The absolute id anchors the probe's FRESHNESS: publish_telemetry
                // reports VALID_PROBE only while the stream has moved less than one
                // full ring past this id, so an old anchor cannot live forever.
                processor_->diag_probe_br.store(start_br, std::memory_order_relaxed);
#endif

                // Freeze the candidate out of the published ring (wrap- and seam-safe),
                // then run the stage-2 snapshot decode on the frozen copy. The ring
                // keeps capturing meanwhile; the snapshot re-checks EVERY row against
                // the LIVE cursor (guarded pre/post-copy) and drops - with a counted
                // reason - any candidate that expired past the overwrite deadline or
                // raced the writer, so no rewritten byte can ever reach the decode.
                wspr_snapshot_t snap;
                int snap_fail = WSPR_SNAP_BAD_ARGS;
                if (!wspr_ring_snapshot_candidate(wspr_portapack_matrix(),
                                                  wspr_portapack_bins(),
                                                  WSPR_RX_BLOCK_ROWS,
                                                  total_br, start_br, col,
                                                  wspr_portapack_bin0_hz(), drift,
                                                  excess, &snap,
                                                  wspr_ring_live_cursor, state_,
                                                  &snap_fail)) {
                    // Expired (the FEC delay let the producer reach the packet's rows)
                    // or overwritten mid-copy; either way the candidate is dropped and
                    // counted, never decoded. scan_.snap_drops stays the total; the
                    // category split feeds the diagnostics only.
                    scan_.snap_drops++;
                    if (snap_fail == WSPR_SNAP_EXPIRED)
                        diag_snap_expired++;
                    else
                        diag_snap_overwritten++;
                    continue;
                }

                // The candidate ENTERED the demod/aggregate stage (snapshot
                // admitted it). This is a stage-entry count, NOT a count of
                // FEC decode calls or FEC failures: everything downstream of
                // this point (demod, aggregation, dedup, association, the
                // >=2-slot report gate) can still drop the candidate. No
                // failure figure may be derived from it by subtracting the
                // M0's report count.
                diag_stage_entries++;

                // Slot-aligned accumulation: demodulate the snapshot's soft
                // weights (no matrix) and fold them into the frequency track
                // of this station. Only an INTEGRATED decode over two or more
                // accumulated slots reports; a single slot never does (the
                // per-window stack decode at low SNR is unreliable).
                int8_t sym_weight[WSPR_CODED_BITS];
                wspr_decode_result_t results[WSPR_MAX_RESULTS]{};
                int num_results = 0;
                if (wspr_snapshot_demod_weights(&snap, sym_weight)) {
                    wspr_decode_result_t integrated;
                    if (wspr_agg_track_contribute(&snap, sym_weight, start_br,
                                                  &integrated)) {
                        results[0] = integrated;
                        num_results = 1;
                    }
                    /* Diagnostic only: where the candidate died. Codes from
                     * wspr_agg_last_result_code(): 1/2/4/6 rejects,
                     * 7 metric/agreement/unpack failure, 0/3/5/8 other. */
                    const int agg_result = wspr_agg_last_result_code();
                    rec_agg_code = (uint8_t)agg_result;
                    switch (agg_result) {
                        case 1:
                        case 2:
                        case 4:
                        case 6:
                            diag_agg_reject++;
                            break;
                        case 7:
                            diag_agg_fail++;
                            break;
                        case 8:
                            /* accepted REPORT: counted by the M0 from the packet */
                            break;
                        default:
                            diag_agg_other++;
                            break;
                    }
                } else {
                    diag_agg_other++;
                }

                // One dedup anchor per REPORTED decode: a failed attempt leaves its
                // neighbourhood scannable, so the aligned bank still gets its chance
                // after the smeared twin failed. A reported decode owns the ghost
                // window around its alignment and frequency.
                if (num_results > 0) {
                    wspr_ring_scan_mark_decoded(&scan_, start_br, col);
                    // An accepted REPORT answers the latest selected candidate.
                    // (v3: reports are counted by the M0 from WSPRPacketMessages.)
                    cand_pending = false;
                }

                for (int i = 0; i < num_results; i++) {
                    // Packet identity rides along: the absolute start block-row and the
                    // tone-0 column identify the PHYSICAL transmission, so the UI can
                    // tell a repeat of the same TX (same start_br) from a new one.
                    shared_memory.application_queue.push(WSPRPacketMessage{
                        results[i].callsign,
                        results[i].grid,
                        results[i].dbm,
                        results[i].snr_db,
                        results[i].freq_hz,
                        results[i].drift_bins,
                        results[i].agreement_pct,
                        start_br,
                        col});
                }

                if (num_results > 0) {
                    last_sync_score = results[0].sync_score;
                }

#if WSPR_RX_DIAG
                // Wall time of the scan + snapshot + decode just finished.
                const systime_t decode_ms = chTimeNow() - decode_t0;
                processor_->diag_decode_ms.store(
                    decode_ms > 65535 ? 65535 : (uint16_t)decode_ms, std::memory_order_relaxed);
                processor_->diag_decode_ms_valid.store(true, std::memory_order_relaxed);
#endif

                // Rich telemetry, pushed just before the status message so the UI
                // writes one debug line carrying both. The ring has no windows and no
                // tracker: row_offset is the absolute start id here, probe fields carry
                // the scanned candidate, last_skip is always zero.
                WSPRDebugMessage::Candidate slots[WSPRDebugMessage::max_candidates]{};
                for (int i = 0; i < num_results && i < (int)WSPRDebugMessage::max_candidates; i++) {
                    slots[i].sync_excess = saturate_i8(results[i].sync_score);
                    slots[i].row_offset = saturate_i8(results[i].row_offset);
                    slots[i].freq_hz = results[i].freq_hz;
                }
                const WSPRDebugMessage debug{
                    analysis_index,
                    (uint8_t)num_results,
                    (uint8_t)num_results,
                    processor_->sync_state_code(),
                    last_sync_score,
                    num_results > 0 ? results[0].metric : (int16_t)0,
                    (int16_t)0,  // no tracker: no phase corrections any more
                    (int16_t)0,
                    slots,
                    (size_t)(num_results > (int)WSPRDebugMessage::max_candidates
                                 ? WSPRDebugMessage::max_candidates
                                 : num_results),
                    // probe_row/probe_score of the debug message carry the candidate
                    // that was just scanned. The absolute start block-row does NOT fit
                    // the int8 slot (it saturates within the first ~50 s of stream) —
                    // the authoritative identity is the WSPRPacketMessage's uint32
                    // start_br above; this byte is only a coarse log hint (its low
                    // 8 bits, the same value %352 that the telemetry rows use).
                    (int8_t)(start_br & 0xFFu),
                    saturate_i8(excess),
                    (int16_t)obs_freq_hz,
                    /* Candidate-level trace outcome: admitted candidate (snap is
                     * always OK here — refused candidates are aggregate-only
                     * via the win line's snapE/snapO), agg = rec_agg_code (255
                     * only if demod failed before contribute). The M0 ignores
                     * this unless WSPR_CANDIDATE_TRACE is on. */
                    rec_agg_code};
                shared_memory.application_queue.push(debug);

                WSPRRxStatusMessage status{
                    (WSPRRxStatusMessage::SyncState)processor_->sync_state_code(),
                    (uint8_t)num_results,
                    last_sync_score};
                shared_memory.application_queue.push(status);
            }
            analysis_index++;
#if WSPR_RX_DIAG
            processor_->diag_window_count.fetch_add(1, std::memory_order_relaxed);
            // Back to capture: the phase publish keeps the UI operation line truthful
            // even when the next periodic tick is far off.
            processor_->publish_telemetry(WSPRTelemetryMessage::Phase::Capture);
#endif
            // Re-read the counter: rows completed while we scanned/decoded.
            total_br = wspr_portapack_total_br(state_);
        } while (wake_pending_.exchange(false, std::memory_order_acq_rel) ||
                 (int32_t)(total_br - wspr_portapack_total_br(state_)) != 0);

        busy_.store(false, std::memory_order_release);

        // Periodic heartbeat, rate-limited well below the per-row signal rate: a
        // quiet band produces NO candidate jobs, and without this the UI would sit
        // on whatever the last candidate-era status said (a stuck Searching) with
        // no way to tell a silent band from a wedged worker. Honest counters only:
        // decode_count is 0 here, the sync state and the scan health counters are
        // current. Never more often than one message pair per interval, and the
        // pushes never block (a full queue drops, as everywhere else).
        const systime_t now = chTimeNow();
        if ((systime_t)(now - last_heartbeat) >= HEARTBEAT_INTERVAL) {
            last_heartbeat = now;
            // Slot-accumulator aging against the live stream position: the
            // accumulator self-ages on candidate jobs, but a quiet band produces
            // none - the heartbeat keeps idle tracks aging in real SLOT units even
            // when nothing transmits (idempotent: wspr_agg_age_to recomputes ages
            // absolutely from each track's newest contribution).
            wspr_agg_age_to(wspr_portapack_total_br(state_));
            // Decode-thread OWN-stack margin: take the address of a heartbeat
            // local (≈ current SP inside run()) and subtract the bottom of this
            // thread's working area. The running minimum is the highwater the
            // D2 hardware check needs — shared_memory.m4_stack_usage tracks the
            // MAIN M4 thread (debug.cpp), never this one.
            volatile const uint8_t sp_marker = 0;
            const uint32_t free_now =
                (uint32_t)&sp_marker - (uint32_t)wspr_decode_thread_wa;
            const uint16_t prev = processor_->diag_decode_stack_free.load(
                std::memory_order_relaxed);
            if (!processor_->diag_decode_stack_valid.load(std::memory_order_relaxed) ||
                free_now < prev) {
                processor_->diag_decode_stack_free.store(
                    free_now > 65535 ? (uint16_t)65535 : (uint16_t)free_now,
                    std::memory_order_relaxed);
                processor_->diag_decode_stack_valid.store(true,
                                                          std::memory_order_relaxed);
            }
            shared_memory.application_queue.push(WSPRRxStatusMessage{
                (WSPRRxStatusMessage::SyncState)processor_->sync_state_code(),
                (uint8_t)0,
                last_sync_score});
#if WSPR_RX_DIAG
            processor_->publish_telemetry(WSPRTelemetryMessage::Phase::Capture);
#endif
        }
    }
}

WSPRRxProcessor::WSPRRxProcessor() {
    decim_0.configure(taps_11k0_decim_0.taps);
    decim_1.configure(taps_11k0_decim_1.taps);
    channel_filter.configure(taps_11k0_channel.taps, 2);

    audio_output.configure(false);

    wspr_portapack_init(&decoder_state);
    // Fresh slot-accumulator tracks per app launch: the multi-slot weight
    // integration must not inherit weights from a previous session's
    // band/frequency.
    wspr_decode_reset_aggregation();

    configured = true;
}

WSPRRxProcessor::~WSPRRxProcessor() {
    wspr_portapack_free(&decoder_state);
}

void WSPRRxProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
    const auto channel_out = channel_filter.execute(decim_1_out, dst_buffer);
    feed_channel_stats(channel_out);
    auto audio = demod_ssb.execute(channel_out, audio_buffer);

    // Decode ahead of the headphone gain stage, so clipping cannot reach the decoder.
    process_wspr_audio(audio);

    for (size_t i = 0; i < audio.count; i++) {
        float sample = audio.p[i] * 2.0f;
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        audio.p[i] = sample;
    }
    audio_output.write(audio);
}

void WSPRRxProcessor::process_wspr_audio(const buffer_f32_t& audio) {
    // 24 kHz to 12 kHz. The 11 kHz channel filter already bandlimits the input, so the
    // decimation aliases nothing back into the 1400-1600 Hz WSPR band.
    static bool decimate_phase = false;

    for (size_t i = 0; i < audio.count; i++) {
        float sample = audio.p[i];

        decimate_phase = !decimate_phase;
        if (!decimate_phase) continue;

        // Inf passes the NaN check, since Inf == Inf holds.
        if (sample != sample || sample > 1e15f || sample < -1e15f) sample = 0.0f;

        // Continuous capture: every sample feeds the ring, nothing is ever skipped or
        // reset. A completed block-row wakes the scan/decode thread; capture continues
        // regardless of what that thread does with the published history.
        if (wspr_portapack_feed_sample(&decoder_state, sample)) {
            decode_thread.signal_row();
        }

#if WSPR_RX_DIAG
        // Periodic live telemetry, at most one message per second, published
        // from this baseband (DMA) thread context - never from an ISR, and
        // publish_telemetry never waits on the queue. The ring captures through
        // the decode, so the phase reads Capture unless a scan/decode is running.
        if (++diag_tick_samples >= 12000) {
            diag_tick_samples = 0;
            publish_telemetry(decode_thread.is_busy() ? WSPRTelemetryMessage::Phase::Decode
                                                      : WSPRTelemetryMessage::Phase::Capture);
        }
#endif
    }
}

#if WSPR_RX_DIAG
void WSPRRxProcessor::publish_telemetry(WSPRTelemetryMessage::Phase phase) {
    /* The flags word MUST be 16-bit: VALID_DIAG is bit 8, and an 8-bit word
     * silently truncates it to 0 - the M0's gated copy of the d_* counters
     * then never runs and every M4 stage counter reads zero forever (the
     * "zero M4 stage counters despite successful reports" defect fixed
     * 2026-11-19). The static_assert keeps the bit honest. */
    static_assert(WSPRTelemetryMessage::VALID_DIAG > 0xFFu,
                  "VALID_DIAG is bit 8: the published flags word must be 16-bit");
    uint16_t flags = 0;
    uint16_t rows = 0;
    uint32_t captured = 0;
    // Ring progress: the physical slot position of the newest block-row, moving once
    // per ~341 ms — the UI's heartbeat that capture is alive. VALID_ROWS keeps its
    // meaning ("capture progress"), now of the ring rather than a window.
    const uint32_t total_br = wspr_portapack_total_br(&decoder_state);
    rows = (uint16_t)(total_br % (uint32_t)WSPR_RX_BLOCK_ROWS);
    // Audio samples fed since launch, the session-elapsed readout.
    captured = decoder_state.total_samples;
    flags |= WSPRTelemetryMessage::VALID_ROWS | WSPRTelemetryMessage::VALID_CAPTURED;

    // Decode wall time: the flag rides on the same "was it ever measured" bit the
    // value does, so the UI shows the number instead of a permanent dm--.
    if (diag_decode_ms_valid.load(std::memory_order_relaxed))
        flags |= WSPRTelemetryMessage::VALID_DECODE_MS;

    // Probe freshness: VALID_PROBE only while the stream has moved less than one
    // full ring past the anchor's absolute id. The ring has no window boundaries
    // and no retune hook inside the running processor, so AGE is the honest expiry:
    // an anchor older than 120 s of stream is gone from the ring and must not
    // arm the UI's SYNC CAND family. A retune/restart recreates the whole
    // processor, which zeroes these atomics anyway.
    int16_t probe_row = diag_probe_row.load(std::memory_order_relaxed);
    const uint32_t probe_br = diag_probe_br.load(std::memory_order_relaxed);
    const bool probe_live =
        probe_row >= 0 &&
        (int32_t)(total_br - probe_br) >= 0 &&
        (int32_t)(total_br - probe_br) <= (int32_t)WSPR_RX_BLOCK_ROWS;
    if (!probe_live)
        probe_row = -1;
    else
        flags |= WSPRTelemetryMessage::VALID_PROBE;

    // Ring scan health: scanned / expired / worst lag / snapshot drops, read
    // unsynchronized from the decode thread's session (counters only).
    const wspr_ring_scan_state_t& sc = decode_thread.scan_state();
    flags |= WSPRTelemetryMessage::VALID_LOSS;
    if (diag_decode_stack_valid.load(std::memory_order_relaxed))
        flags |= WSPRTelemetryMessage::VALID_STACK;

    // Latest selected-candidate observation (VALID_CAND, v3): the flag IS the
    // pending state ("M4 selected a candidate, no accepted REPORT yet"); the
    // 150 s freshness/expiry window is owned by the M0/UI side (age from the
    // message arrival tick). ONE field: cand_freq_hz.
    const bool cand_live = decode_thread.cand_pending;
    if (cand_live) flags |= WSPRTelemetryMessage::VALID_CAND;

    // Bounded aggregate counters (VALID_DIAG): always meaningful (cumulative
    // since session start), published every time.
    flags |= WSPRTelemetryMessage::VALID_DIAG;

    const WSPRTelemetryMessage message{
        phase,
        flags,
        diag_seq.fetch_add(1, std::memory_order_relaxed),
        rows,
        captured,
        0,  // no rephase skips exist in the ring
        diag_decode_ms_valid.load(std::memory_order_relaxed)
            ? diag_decode_ms.load(std::memory_order_relaxed)
            : (uint16_t)0,
        (uint8_t)diag_window_count.load(std::memory_order_relaxed),
        (uint16_t)diag_telem_dropped.load(std::memory_order_relaxed),
        saturate_i8(probe_row),
        saturate_i8(diag_probe_score.load(std::memory_order_relaxed)),
        sc.scanned,
        sc.expired,
        sc.max_lag,
        sc.snap_drops,
        diag_decode_stack_valid.load(std::memory_order_relaxed)
            ? diag_decode_stack_free.load(std::memory_order_relaxed)
            : (uint16_t)0,
        decode_thread.obs_freq_hz,
        // VALID_DIAG: candidates released by the scan (counted at the single
        // release funnel - NOT derivable from message counts, because the
        // periodic heartbeat also publishes in the Decode phase) and the
        // demod/aggregate stage entries. Snapshot/drop totals ride VALID_LOSS.
        decode_thread.diag_cand_released,
        decode_thread.diag_stage_entries,
        decode_thread.diag_snap_expired,
        decode_thread.diag_snap_overwritten,
        decode_thread.diag_agg_reject,
        decode_thread.diag_agg_fail,
        decode_thread.diag_agg_other};
    if (!shared_memory.application_queue.push(message))
        diag_telem_dropped.fetch_add(1, std::memory_order_relaxed);
}
#endif

uint8_t WSPRRxProcessor::sync_state_code() const {
    // The window tracker is gone; the ring is always capturing. Searching while the
    // ring still fills, Syncing while a scan/decode pass runs, Locked once the ring
    // holds the full 120 s of history.
    if (wspr_portapack_valid_rows(&decoder_state) < (uint32_t)WSPR_RX_BLOCK_ROWS)
        return (uint8_t)WSPRRxStatusMessage::SyncState::Searching;
    return (uint8_t)(decode_thread.is_busy() ? WSPRRxStatusMessage::SyncState::Syncing
                                             : WSPRRxStatusMessage::SyncState::Locked);
}

void WSPRRxProcessor::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::CaptureConfig:
            capture_config(*reinterpret_cast<const CaptureConfigMessage*>(message));
            break;

        case Message::ID::WSPRRestart:
            /* RX RESTART, producer side: runs on the baseband message thread
             * BETWEEN capture buffers - the producer is quiescent here, so
             * resetting its state cannot tear an in-flight row. Matrix,
             * banks and the row counters restart from zero; the next capture
             * second is the new session's t=0. The decode thread resets its
             * own session state via request_session_restart(). */
            wspr_portapack_init(&decoder_state);
            diag_probe_br.store(0, std::memory_order_relaxed);
            diag_probe_row.store(-1, std::memory_order_relaxed);
            decode_thread.request_session_restart();
            break;

        default:
            break;
    }
}

void WSPRRxProcessor::capture_config(const CaptureConfigMessage& message) {
    if (message.config) {
        audio_output.set_stream(std::make_unique<StreamInput>(message.config));
    } else {
        audio_output.set_stream(nullptr);
    }
}

int main() {
    audio::dma::init_audio_out();

    EventDispatcher event_dispatcher{std::make_unique<WSPRRxProcessor>()};
    event_dispatcher.run();
    return 0;
}
