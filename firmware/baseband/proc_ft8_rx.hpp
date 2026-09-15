/*
 * Copyright (C) 2026 Dmytro Onyshko
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

#ifndef __PROC_FT8_RX_H__
#define __PROC_FT8_RX_H__

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
#include "ft8_portapack.h"

#include <ch.h>
#include <atomic>

class FT8RxProcessor;

// Worker thread that runs the candidate search and LDPC passes off the DMA-driven
// baseband thread. The baseband thread signals a semaphore when the analysis window is
// full; this thread wakes up, decodes, hands the Costas measurement back to the
// processor's sync loop, pushes messages to M0, and clears the waterfall so the
// baseband thread can start filling the next window.
class FT8DecodeThread : public ThreadBase {
   public:
    FT8DecodeThread(ft8_decoder_state_t* state, FT8RxProcessor* processor);
    ~FT8DecodeThread();

    // Owns a ChibiOS thread and borrows two objects that outlive it; copying either
    // would duplicate that ownership.
    FT8DecodeThread(const FT8DecodeThread&) = delete;
    FT8DecodeThread& operator=(const FT8DecodeThread&) = delete;

    void start() override;

    // Called from baseband thread when a slot is ready to decode.
    // Returns false (no-op) if decoder is still busy with the previous slot.
    bool signal_decode();

    // True while the decoder thread is actively processing a slot.
    bool is_busy() const { return busy_.load(std::memory_order_acquire); }

   private:
    void run() override;

    ft8_decoder_state_t* state_;
    FT8RxProcessor* processor_;
    BinarySemaphore sem_{};
    std::atomic<bool> busy_{false};
    std::atomic<bool> terminate_{false};

    static Thread* thread_;
};

class FT8RxProcessor : public BasebandProcessor {
   public:
    FT8RxProcessor();
    ~FT8RxProcessor();

    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    static constexpr size_t baseband_fs = 3072000;  // 3.072 MHz input

    // Decimation chain: 3.072 MHz → 384 kHz → 48 kHz → 24 kHz (like AFSK RX)
    static constexpr size_t decim_0_output_fs = baseband_fs / 8;  // 384 kHz
    static constexpr size_t decim_1_output_fs = decim_0_output_fs / 8;  // 48 kHz
    static constexpr size_t audio_fs = 24000;  // 24 kHz audio (48k / 2 via channel_filter)

    // Buffers
    std::array<complex16_t, 512> dst{};
    const buffer_c16_t dst_buffer{
        dst.data(),
        dst.size()};

    std::array<float, 32> audio{};
    const buffer_f32_t audio_buffer{
        audio.data(),
        audio.size()};

    // Decimation chain (like AFSK RX: 8x8x2 = 128x total)
    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};
    dsp::decimate::FIRC16xR16x32Decim8 decim_1{};
    dsp::decimate::FIRAndDecimateComplex channel_filter{};  // decimation=2 -> 24kHz output

    // FT8 is received as USB, so the SSB demodulator is the only one this app needs.
    dsp::demodulate::SSB demod_ssb{};

    // Monitor audio only; the decoder taps the signal ahead of any gain stage.
    AudioOutput audio_output{};

    bool configured{false};

    // FT8 decoder state
    // No audio_accumulator needed — Goertzel processes samples incrementally
    ft8_decoder_state_t decoder_state{};
    uint32_t slot_count{0};

    // --- Slot synchronisation ---
    //
    // The receiver does not trust the RTC. It captures a window longer than a
    // transmission, asks the Costas correlator which block the transmission actually
    // started in, and moves the window so that block becomes TARGET_OFFSET. Wall-clock
    // time never enters the loop: every station on the band keys to the same T/R
    // boundary, so the band itself is the time reference.
    enum class SyncState : uint8_t {
        Acquire,  // Nothing found yet; step the capture phase until the correlator bites
        Fine,     // Block grid locked; walk the sub-symbol phase for the strongest score
        Track,    // Locked; re-centre the window every slot
    };

    static constexpr int32_t SLOT_SAMPLES = 15 * FT8_SAMPLE_RATE;  // 180000 = 15.000 s
    static constexpr int32_t CAPTURE_SAMPLES = FT8_WATERFALL_BLOCKS * FT8_SAMPLES_PER_SYMBOL;
    // What is left of the slot after the capture. The decoder runs here: measured
    // 0.87 ms on an x86 host for find_candidates plus all LDPC passes, which scales to
    // roughly 50-150 ms on this M4, so 440 ms leaves a wide margin.
    static constexpr int32_t BASE_GAP = SLOT_SAMPLES - CAPTURE_SAMPLES;  // 5280 = 440 ms
    // Park the transmission in the middle of the window so drift in either direction
    // has equal slack before a symbol falls outside the buffer.
    static constexpr int16_t TARGET_OFFSET = (FT8_WATERFALL_BLOCKS - FT8_NUM_SYMBOLS) / 2;

    // Costas score below which a slot counts as carrying nothing usable. This gates
    // tracking only; acquisition needs a CRC-valid decode, because a window misaligned by
    // one Costas period scores just as well as the real thing (see ft8_portapack.h).
    static constexpr int16_t SYNC_MIN_SCORE = 18;
    // One symbol walked in six steps. time_offset is quantised to whole blocks because
    // adjacent blocks analyse disjoint 1920-sample windows, so the sub-symbol residual
    // cannot be interpolated out of the score; it has to be found by moving the window
    // and comparing. Measured on the host: the score peaks at 44 in phase against 11 at
    // half a symbol out, which makes the peak unambiguous.
    static constexpr int32_t DITHER_STEP = FT8_SAMPLES_PER_SYMBOL / 6;  // 320
    static constexpr uint8_t DITHER_STEPS = 6;
    // A correction does not reach the capture immediately. next_slot_gap() consumes it
    // at the end of the NEXT window, so it positions the one after that: every phase
    // change takes effect two windows after it is computed. Both the dither search and
    // the tracking loop have to account for that or they act on stale measurements.
    static constexpr uint8_t SYNC_LATENCY_SLOTS = 2;
    // Cold search step. ftx_find_candidates spans time_offset -10..+19 blocks, about
    // 4.6 s of capture phase, so 4 s steps cover the 15 s slot in four positions.
    // Each position is held long enough for the shift to land and be judged.
    static constexpr int32_t SWEEP_STEP = 4 * FT8_SAMPLE_RATE;
    static constexpr uint8_t SWEEP_SLOTS = SYNC_LATENCY_SLOTS + 1;
    // Silence tolerated before dropping the lock, and degraded slots before re-running
    // the sub-symbol search. At 20 ppm the clock walks a full symbol in about 2 hours.
    static constexpr uint8_t TRACK_LOST_SLOTS = 8;
    static constexpr uint8_t WEAK_SLOTS_LIMIT = 4;

    SyncState sync_state{SyncState::Acquire};
    uint8_t dither_index{0};
    uint8_t dither_best{0};
    int16_t dither_best_score{0};
    int16_t lock_score{0};
    uint8_t quiet_slots{0};
    uint8_t weak_slots{0};
    uint8_t settle_slots{0};  // Windows to ignore while a correction is still in flight

    // Written by FT8DecodeThread, consumed by the baseband thread at the next slot end.
    // Routing every phase change through this keeps skip_samples single-writer.
    std::atomic<int32_t> pending_phase_adjust{0};

    uint32_t skip_samples{0};  // Samples to drop before the next capture, at 12 kHz

    // Message handlers
    void capture_config(const CaptureConfigMessage& message);
    void configure_threshold(const FT8ConfigureMessage& message);

    // FT8 processing
    void process_ft8_audio(const buffer_f32_t& audio);

   public:
    // Runs on FT8DecodeThread once the slot has been decoded: turns the Costas
    // measurement into the next capture-phase correction.
    void on_slot_decoded();
    uint8_t sync_state_code() const { return (uint8_t)sync_state; }

   private:
    uint32_t next_slot_gap();

    /* NB: Threads should be the last members in the class definition. */
    FT8DecodeThread decode_thread{&decoder_state, this};
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};
};

#endif  // __PROC_FT8_RX_H__
