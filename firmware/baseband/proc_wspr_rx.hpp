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

#ifndef __PROC_WSPR_RX_H__
#define __PROC_WSPR_RX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"

#include <optional>
#include "rssi_thread.hpp"
#include "thread_base.hpp"

#include "dsp_iir.hpp"
#include "dsp_iir_config.hpp"
#include "audio_output.hpp"
#include "wspr_frontend.hpp"

#include "message.hpp"
#include "wspr_portapack.h"
#include "wspr_lib/wspr_decode.h"
#include "wspr_session.hpp"

#include <ch.h>
#include <atomic>

// WSPR_RX_DIAG: on-air diagnostics build (the WSPRTelemetryMessage snapshot).
// The top-level CMake always defines this to 0 (ordinary release) or 1
// (WSPR_DIAGNOSTIC=ON) for BOTH the M0 application and the M4 baseband, because
// they share message.hpp. The #ifndef default is 0 so a standalone compile of
// this file is the ordinary release build; only CMake enables diagnostics.
// Everything diagnostic lives under #if WSPR_RX_DIAG: the diag members/atomics,
// wspr_diag_counters.hpp, publish_telemetry, the stack sample, the ~1 Hz
// telemetry producer and every push-failure counter. The release image must
// reference no WSPRTelemetryMessage and no diag symbol.
#ifndef WSPR_RX_DIAG
#define WSPR_RX_DIAG 0
#endif

#if WSPR_RX_DIAG
#include "wspr_diag_counters.hpp"
#endif

class WSPRRxProcessor;

// Ring-architecture decode thread: incremental scan of ONE new start hypothesis per
// completed block-row, snapshot decode of the candidates that pass the sync gate. The
// ring never stops capture, so this thread runs entirely on PUBLISHED history and the
// seconds-long sequential decode delays (but never blocks) the receiver.
class WSPRDecodeThread : public ThreadBase {
   public:
    WSPRDecodeThread(wspr_portapack_state_t* state, WSPRRxProcessor* processor);
    ~WSPRDecodeThread();

    WSPRDecodeThread(const WSPRDecodeThread&) = delete;
    WSPRDecodeThread& operator=(const WSPRDecodeThread&) = delete;

    void start() override;

    /* Stop and join the decode thread. Called explicitly by the processor
     * destructor BEFORE the producer state is released; idempotent. */
    void stop();

    // Wakes the scan/decode loop. Call once per completed block-row; internally
    // coalesced, so a signal is a wakeup, not a per-row token.
    void signal_row();

    // True while a scan or a snapshot decode is running (telemetry DECODE phase).
    bool is_busy() const { return busy_.load(std::memory_order_acquire); }

    // RX RESTART / RETUNE: the producer (WSPRRxProcessor::execute) performs the
    // in-place reset at a buffer boundary and publishes a new session
    // generation. This thread never receives a reset flag: it compares
    // processor_->session_generation() at every scan-job boundary, drops any
    // in-flight result from an older generation, and resets its own scan
    // session + slot accumulator when it adopts the new one. That keeps the
    // matrix memset on the producer thread and the decode-thread state on
    // this one, with no lock and no cross-thread teardown.
    // The scan session's counters for diagnostics/telemetry. Deliberately
    // unsynchronized: telemetry reads them from another thread, counters only
    // (a torn uint32 read can only misreport a diagnostics value).
    // DIAGNOSTIC BUILD ONLY: nothing in the release path reads it.
#if WSPR_RX_DIAG
    const wspr_ring_scan_state_t& scan_state() const { return scan_; }

    // ---- Observational diagnostics (VALID_DIAG telemetry) ----
    // ONE decode-stage counter kept on the M4: candidate jobs that ENTERED
    // the demod/aggregate stage (the snapshot admitted them). It is NOT an
    // FEC-call or FEC-failure count: everything downstream of the entry point
    // (demod, aggregation, dedup, association, the >=2-slot report gate) can
    // still drop the candidate, and no failure figure may be derived from it
    // by subtracting the M0's report count.
    // Written ONLY by the decode thread; publish_telemetry reads it
    // unsynchronized (counters only, same honesty contract as scan_state()).
    // No decision path reads any of it. Zeroed on RX RESTART in run()
    // (session reset) and at construction; cumulative uint32 afterwards,
    // wrap = natural uint32 wrap. d_cand_selected comes from the scan
    // state's counters; snapshot/agg categories are no longer
    // distinguished (snap total stays in VALID_LOSS scan_snap_drops).
    uint32_t diag_cand_released{0};  // scan releases (single funnel site)
    uint32_t diag_stage_entries{0};  // demod/aggregate stage entries (was diag_fec_attempts)
    /* Where candidates die (all observational). Snapshot drops are split into
     * the four WSPR_SNAP_* reasons; scan_.snap_drops stays the total. */
    uint32_t diag_snap_expired{0};
    uint32_t diag_snap_overwritten{0};
    uint32_t diag_snap_weak{0};
    uint32_t diag_snap_bad_args{0};
    /* Agg reason classification (T01): ONE instance, filled by
     * wspr::diag_counters::classify() after every contribute call, reset on RX
     * RESTART. Read unsynchronized by publish_telemetry (counters only, the
     * same honesty contract as scan_state()); no decision path reads it. */
    wspr::diag_counters::AggCounters agg_counters{};
    // Latest selected-candidate observation (VALID_CAND): set by the
    // decode thread each time wspr_ring_scan_poll releases a candidate;
    // cleared on an accepted REPORT. NO M4-side freshness window - the M0/UI
    // owns the 150 s expiry from its own arrival tick. Only the tone-0
    // frequency is published (excess rides the existing probe channel).
    uint16_t obs_freq_hz{0};
    bool cand_pending{false};
#endif

   private:
    void run() override;

    wspr_portapack_state_t* state_;
    WSPRRxProcessor* processor_;
    BinarySemaphore sem_{};
    std::atomic<bool> wake_pending_{false};
    std::atomic<bool> busy_{false};
    std::atomic<bool> terminate_{false};

    // The ring scan session: pending start hypotheses, dedup anchor, counters.
    wspr_ring_scan_state_t scan_{};

    static Thread* thread_;
};

class WSPRRxProcessor : public BasebandProcessor {
   public:
    WSPRRxProcessor();
    ~WSPRRxProcessor();

    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

    // ---- Capture-session boundaries (T05) ----
    // Called from the baseband event thread when WSPRRestartMessage arrives.
    // ONLY records the request; the producer's execute() performs the reset at
    // the next buffer boundary. The request id is echoed back in
    // WSPRRxStatusMessage::restart_ack once the reset has actually completed.
    void request_session_restart(uint16_t request_id);
    // Generation currently owned by the producer (0 = process launch session).
    uint32_t session_generation() const { return session_.generation(); }
    // Consistent (generation, ack) snapshot; the decode thread uses ONE view
    // per adoption so a status can never pair an old session with a new ack.
    wspr::session::Session::View session_view() const { return session_.view(); }
    // Request id acknowledged by the latest completed reset (0xFFFF = none).
    uint16_t session_ack() const { return session_.ack_id(); }

   private:
    static constexpr size_t baseband_fs = 3072000;

    // 3.072 MHz -> 384 kHz -> 48 kHz -> 24 kHz, the same chain AFSK RX and FT8 RX use.
    static constexpr size_t decim_0_output_fs = baseband_fs / 8;
    static constexpr size_t decim_1_output_fs = decim_0_output_fs / 8;
    static constexpr size_t audio_fs = 24000;

    std::array<complex16_t, 512> dst{};
    const buffer_c16_t dst_buffer{
        dst.data(),
        dst.size()};

    std::array<float, 32> audio{};
    const buffer_f32_t audio_buffer{
        audio.data(),
        audio.size()};

    // Shared, testable production frontend: 3.072 MHz c8 -> 24 kHz real, with
    // the T07 complex USB stage. The host harness instantiates this same class
    // (and the same taps) so the measured chain is the firmware chain.
    wspr::Frontend frontend_{};

    AudioOutput audio_output{};

    bool configured{false};

    wspr_portapack_state_t decoder_state{};

    // Producer-owned session handshake (request recorded by the event thread,
    // reset + generation committed here on the capture thread).
    wspr::session::Session session_{};

    // 24 kHz -> 12 kHz sample selector. A member (not a function-static) so a
    // new capture session starts at a defined phase.
    bool decimate_phase_{false};

#if WSPR_RX_DIAG
   public:
    // ---- WSPR_RX_DIAG live telemetry (WSPRTelemetryMessage) ----
    // Sequence numbers, counters and the last analysis wall time are shared by
    // the baseband thread (periodic publishes) and the decode thread (per-analysis
    // publishes), hence the atomics. diag_tick_samples is baseband-thread private.
    std::atomic<uint16_t> diag_seq{0};
    // Only pairing key for the per-analysis snapshot: incremented once per
    // diagnostic analysis by the decode thread, and merely READ (never
    // incremented) by the baseband thread's periodic publish.
    std::atomic<uint16_t> diag_analysis_index{0};
    std::atomic<uint16_t> diag_decode_ms{0};
    std::atomic<bool> diag_decode_ms_valid{false};
    std::atomic<uint32_t> diag_telem_dropped{0};
    // Application-queue refusals of the non-telemetry pushes (T01). Written by
    // the decode thread for packets/status and read by the baseband thread
    // in publish_telemetry, hence atomics. Best-effort by design: a refused push
    // is counted and never retried or waited on.
    std::atomic<uint32_t> diag_push_fail_report{0};  // WSPRPacketMessage refused
    std::atomic<uint32_t> diag_push_fail_aux{0};     // status refused
    std::atomic<int16_t> diag_probe_row{-1};
    std::atomic<int16_t> diag_probe_score{0};
    // Absolute block-row id the probe anchor was taken at: publish_telemetry
    // treats the probe as VALID only while it is younger than one full ring
    // (WSPR_RX_BLOCK_ROWS), so an hours-old anchor can never re-arm
    // VALID_PROBE (retune/restart clears the whole processor state anyway).
    std::atomic<uint32_t> diag_probe_br{0};
    // Decode-thread OWN-stack FREE margin at the heartbeat sample point, bytes
    // still free inside wspr_decode_thread_wa. The sample is the address of a
    // run()-loop local (the current SP). It is ONE sample, not a high-water
    // mark: the deeper snapshot/FEC call frames are NOT included, so the
    // sampled free margin can be LARGER than the true minimum free margin.
    // Peak USED stack is only lower-bounded by this sample; this value is NOT
    // a lower bound on the minimum free margin and NOT an FEC-chain
    // high-water mark. Running minimum since launch; the shared
    // m4_stack_usage slot tracks the MAIN M4 thread and must not be
    // repurposed.
    std::atomic<uint16_t> diag_decode_sp_free_min{0};
    std::atomic<bool> diag_decode_sp_valid{false};
    uint32_t diag_tick_samples{0};  // 12 kHz fed-sample counter, 1 s cadence

    // Per-analysis observation carried by the diagnostic telemetry snapshot.
    // Built by the decode thread once per admitted candidate analysis; the
    // periodic 1 Hz publish passes nullptr and leaves these fields at zero.
    struct AnalysisSnapshot {
        uint16_t analysis_index;
        uint8_t decode_count;
        int16_t sync_score;
        uint8_t num_candidates;
        int16_t best_metric;
        int8_t probe_row;
        int8_t probe_score;
        WSPRTelemetryMessage::Candidate candidates[WSPRTelemetryMessage::max_candidates];
        size_t candidate_count;
    };

    // Builds and pushes one telemetry message from whichever thread calls it.
    // Never blocks: a full application queue drops the message and counts the
    // drop, and the DMA/receive path is never delayed by diagnostics.
    // `analysis` is non-null only for the per-analysis snapshot (VALID_ANALYSIS);
    // the periodic snapshot passes nullptr and leaves analysis_index unchanged.
    void publish_telemetry(WSPRTelemetryMessage::Phase phase,
                           const AnalysisSnapshot* analysis = nullptr);
#endif

   private:
    void capture_config(const CaptureConfigMessage& message);
    void process_wspr_audio(const buffer_f32_t& audio);
    // Capture thread only: clear the ring/banks and the decimator history for a
    // new session. Never called from the event thread.
    void reset_capture_chain();

   public:
    // Compact status code the UI sees, in BOTH builds.
    // Filling while the ring does not yet hold WSPR_RX_BLOCK_ROWS rows;
    // otherwise Decoding while a scan/decode pass is running (NOT a statement
    // that FEC succeeded), else Receiving (full ring, decode thread idle).
    WSPRRxStatusMessage::SyncState sync_state_code() const;

    // ring_rows for WSPRRxStatusMessage: absolute stream position % 352
    // (0..351), the physical block-row position inside the ring.
    uint16_t ring_rows() const;

   private:
    /* NB: Threads should be the last members in the class definition. */
    WSPRDecodeThread decode_thread{&decoder_state, this};
    std::optional<BasebandThread> baseband_thread{};
    RSSIThread rssi_thread{};
};

#endif  // __PROC_WSPR_RX_H__
