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

    // True while a scan or a snapshot decode is running .
    bool is_busy() const { return busy_.load(std::memory_order_acquire); }

    // RX RESTART / RETUNE: the producer (WSPRRxProcessor::execute) performs the
    // in-place reset at a buffer boundary and publishes a new session
    // generation. This thread never receives a reset flag: it compares
    // processor_->session_generation() at every scan-job boundary, drops any
    // in-flight result from an older generation, and resets its own scan
    // session + slot accumulator when it adopts the new one. That keeps the
    // matrix memset on the producer thread and the decode-thread state on
    // this one, with no lock and no cross-thread teardown.
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
