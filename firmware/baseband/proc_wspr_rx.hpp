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

class WSPRRxProcessor;

// Runs the candidate search and the unpacking passes off the DMA-driven baseband thread,
// which signals the semaphore once the capture window is full.
class WSPRDecodeThread : public ThreadBase {
   public:
    WSPRDecodeThread(wspr_portapack_state_t* state, WSPRRxProcessor* processor);
    ~WSPRDecodeThread();

    WSPRDecodeThread(const WSPRDecodeThread&) = delete;
    WSPRDecodeThread& operator=(const WSPRDecodeThread&) = delete;

    void start() override;

    // Returns false when the previous window is still being decoded.
    bool signal_decode();

    bool is_busy() const { return busy_.load(std::memory_order_acquire); }

   private:
    void run() override;

    wspr_portapack_state_t* state_;
    WSPRRxProcessor* processor_;
    BinarySemaphore sem_{};
    std::atomic<bool> busy_{false};
    std::atomic<bool> terminate_{false};

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

    // The T/R slot is 120 s and a transmission about 110.6 s, but the receiver cannot know
    // where in the slot the transmission started, so the capture window is longer than the
    // slot: 176 rows of 8192 samples = 120.13 s. A window that is longer than the slot
    // contains any transmission completely no matter where it starts, which is what makes
    // acquisition free: no sweeping is needed, the decoder finds the transmission in the
    // 14 rows of slack.
    static constexpr int32_t SLOT_SAMPLES = 120 * WSPR_SAMPLE_RATE;  // 1440000
    static constexpr int32_t WINDOW_SAMPLES =
        WSPR_RX_NUM_ROWS * WSPR_SAMPLES_PER_SYMBOL;  // 1441792
    // How much later in the slot each back-to-back window starts than the one before it.
    // The phase walks this far per window plus the decode overrun, so the row offset the
    // decoder reports slides down by roughly half a row per window, and the tracker below
    // periodically jumps it back up.
    static constexpr int32_t WINDOW_OVERHANG = WINDOW_SAMPLES - SLOT_SAMPLES;
    static_assert(WINDOW_OVERHANG == 1792, "Window length vs slot length changed");
    static_assert(WINDOW_OVERHANG < SLOT_SAMPLES, "Window must fit near the slot");

    // Slot tracking, the FT8 loop adapted. A confirmed decode is the only alignment
    // proof, so like FT8 the tracker trusts decodes alone. When the reported row offset
    // reaches the bottom of the slack the next jump would come too late, so the tracker
    // jumps the window phase forward by (LAND_ROW - row) rows: the skip is one slot minus
    // the overhang minus the jump, about 111 s of dead air, and the transmission lands
    // near the end of the slack with the maximum drift room ahead of it. Between jumps
    // the loop dithers the sub-row phase by one sixth of a row on failed windows, the
    // same walk FT8 does, because a row grid that is off the symbol boundaries smears
    // every symbol across two tones.
    static constexpr int16_t TRACK_LAND_ROW = 13;
    static constexpr int32_t DITHER_STEP = WSPR_SAMPLES_PER_SYMBOL / 6;
    static constexpr uint8_t QUIET_LIMIT = 4;         // locked -> reacquire after this many empty windows
    static constexpr uint8_t ACQUIRE_JUMP_QUIET = 3;  // unlocked -> phase jump after this many empty windows
    // Sub-row chase thresholds: a window whose best decode agreement reaches this    // holds the capture phase; weaker windows walk one dither step.
    static constexpr int8_t WSPR_AGREEMENT_HOLD = 80;

    std::atomic<int32_t> pending_skip{0};  // decode thread -> baseband thread, in samples
    uint32_t skip_samples{0};              // consumed at 12 kHz by the baseband thread

    // Written only by the decode thread.
    bool locked{false};
    uint8_t quiet_windows{0};
    uint8_t acquire_quiet{0};  // empty windows since the last acquisition jump
    // Best sync score seen while decodes kept coming: the reference the sub-row
    // phase chase steers toward (see on_window_decoded).
    int16_t best_score_seen{0};

    void capture_config(const CaptureConfigMessage& message);
    void process_wspr_audio(const buffer_f32_t& audio);

   public:
    // Runs on the decode thread: turns the reported row offset into the next capture
    // phase correction and the status the UI sees. Returns the correction stored to
    // pending_skip (0, a dither step, or the slot jump saturated to int16), so the
    // debug telemetry can carry it without a second shared-memory round trip.
    int16_t on_window_decoded(uint8_t num_results, const wspr_decode_result_t* results);
    uint8_t sync_state_code() const;
    // The phase chase reference for the debug telemetry; the decode thread just
    // updated it when this is read, so the value is current.
    int16_t tracker_best_score() const { return best_score_seen; }

   private:
    /* NB: Threads should be the last members in the class definition. */
    WSPRDecodeThread decode_thread{&decoder_state, this};
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};
};

#endif  // __PROC_WSPR_RX_H__
