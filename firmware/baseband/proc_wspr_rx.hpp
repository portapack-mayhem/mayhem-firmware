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
#include "rssi_thread.hpp"
#include "thread_base.hpp"

#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "dsp_iir.hpp"
#include "dsp_iir_config.hpp"
#include "audio_output.hpp"

#include "message.hpp"
#include "wspr_portapack.h"
#include "wspr_lib/wspr_decode.h"

#include <ch.h>
#include <atomic>

// WSPR_RX_DIAG: temporary on-air diagnostics build (live telemetry message and
// the UI diagnostic lines). Removal later means deleting every block marked
// WSPR_RX_DIAG in proc_wspr_rx.{hpp,cpp}, ui_wspr_rx.{hpp,cpp} and
// wspr_diag.hpp - nothing else depends on it.
#ifndef WSPR_RX_DIAG
#define WSPR_RX_DIAG 1
#endif

// WSPR_CANDIDATE_TRACE: temporary per-candidate diagnostic logging. When 1 the
// decode thread stamps each candidate's FINAL disposition (freq/snap/agg) onto
// the EXISTING WSPRDebugMessage it already pushes. Default 0 keeps the disabled
// build free of the hot-path recording. Mirrors the WSPR_RX_DIAG pattern above;
// no CMake define is emitted, the #ifndef default is the single source of truth.
#ifndef WSPR_CANDIDATE_TRACE
#define WSPR_CANDIDATE_TRACE 0
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

    // Wakes the scan/decode loop. Call once per completed block-row; internally
    // coalesced, so a signal is a wakeup, not a per-row token.
    void signal_row();

    // True while a scan or a snapshot decode is running (telemetry DECODE phase).
    bool is_busy() const { return busy_.load(std::memory_order_acquire); }

    // RX RESTART (UI button): ask the decode thread to reset what IT owns -
    // the ring scan session and the slot accumulator - at its next loop top.
    // Memory-safe without locks: fixed arrays, and every reset is a memset of
    // thread-private state. The producer-owned decoder_state/matrix is reset
    // by the processor on the message thread (see on_message).
    void request_session_restart() {
        restart_request_.store(true, std::memory_order_release);
        wake_pending_.store(true, std::memory_order_release);
        chBSemSignal(&sem_);
    }

    // The scan session's counters for diagnostics/telemetry. Deliberately
    // unsynchronized: telemetry reads them from another thread, counters only
    // (a torn uint32 read can only misreport a diagnostics value).
    const wspr_ring_scan_state_t& scan_state() const { return scan_; }

    // ---- Observational diagnostics (VALID_DIAG telemetry, CONTRACT v3) ----
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
    /* Where candidates die (all observational). */
    uint32_t diag_snap_expired{0};
    uint32_t diag_snap_overwritten{0};
    uint32_t diag_agg_reject{0};
    uint32_t diag_agg_fail{0};
    uint32_t diag_agg_other{0};
    // Latest selected-candidate observation (VALID_CAND, v3): set by the
    // decode thread each time wspr_ring_scan_poll releases a candidate;
    // cleared on an accepted REPORT. NO M4-side freshness window - the M0/UI
    // owns the 150 s expiry from its own arrival tick. Only the tone-0
    // frequency is published (excess rides the existing probe channel).
    uint16_t obs_freq_hz{0};
    bool cand_pending{false};

   private:
    void run() override;

    wspr_portapack_state_t* state_;
    WSPRRxProcessor* processor_;
    BinarySemaphore sem_{};
    std::atomic<bool> wake_pending_{false};
    std::atomic<bool> busy_{false};
    std::atomic<bool> terminate_{false};
    std::atomic<bool> restart_request_{false};

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

    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};
    dsp::decimate::FIRC16xR16x32Decim8 decim_1{};
    dsp::decimate::FIRAndDecimateComplex channel_filter{};

    // WSPR is received as USB, so SSB is the only demodulator this app needs.
    dsp::demodulate::SSB demod_ssb{};

    AudioOutput audio_output{};

    bool configured{false};

    wspr_portapack_state_t decoder_state{};

#if WSPR_RX_DIAG
   public:
    // ---- WSPR_RX_DIAG live telemetry (WSPRTelemetryMessage) ----
    // Sequence numbers, counters and the last analysis wall time are shared by
    // the baseband thread (periodic publishes) and the decode thread (scan/decode
    // publishes), hence the atomics. diag_tick_samples is baseband-thread private.
    std::atomic<uint16_t> diag_seq{0};
    std::atomic<uint32_t> diag_window_count{0};
    std::atomic<uint16_t> diag_decode_ms{0};
    std::atomic<bool> diag_decode_ms_valid{false};
    std::atomic<uint32_t> diag_telem_dropped{0};
    std::atomic<int16_t> diag_probe_row{-1};
    std::atomic<int16_t> diag_probe_score{0};
    // Absolute block-row id the probe anchor was taken at: publish_telemetry
    // treats the probe as VALID only while it is younger than one full ring
    // (WSPR_RX_BLOCK_ROWS), so an hours-old anchor can never re-arm
    // VALID_PROBE (retune/restart clears the whole processor state anyway).
    std::atomic<uint32_t> diag_probe_br{0};
    // Decode-thread OWN-stack highwater, bytes still free inside
    // wspr_decode_thread_wa (running minimum since launch). Updated by the
    // decode thread itself in its heartbeat; the shared m4_stack_usage slot
    // tracks the MAIN M4 thread and must not be repurposed for this.
    std::atomic<uint16_t> diag_decode_stack_free{0};
    std::atomic<bool> diag_decode_stack_valid{false};
    uint32_t diag_tick_samples{0};  // 12 kHz fed-sample counter, 1 s cadence

    // Builds and pushes one telemetry message from whichever thread calls it.
    // Never blocks: a full application queue drops the message and counts the
    // drop, and the DMA/receive path is never delayed by diagnostics.
    void publish_telemetry(WSPRTelemetryMessage::Phase phase);
#endif

   private:
    void capture_config(const CaptureConfigMessage& message);
    void process_wspr_audio(const buffer_f32_t& audio);

   public:
    // Status code the UI sees. The phase tracker is gone with the window: the ring is
    // always capturing, so the state reads Locked once the ring holds a full 352 rows
    // and Searching while it is still filling (plus Syncing while a scan/decode runs).
    uint8_t sync_state_code() const;

   private:
    /* NB: Threads should be the last members in the class definition. */
    WSPRDecodeThread decode_thread{&decoder_state, this};
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};
};

#endif  // __PROC_WSPR_RX_H__
