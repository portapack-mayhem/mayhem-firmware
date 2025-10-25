/*
 * Copyright (C) 2025 StarVore Labs
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

#ifndef __PROC_SSTV_RX_H__
#define __PROC_SSTV_RX_H__

#include "portapack_shared_memory.hpp"
#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "sstv.hpp"
#include "dsp_fir_taps.hpp"
#include <complex>

using namespace sstv;

class SSTVRXProcessor : public BasebandProcessor {
   public:
    SSTVRXProcessor() : state(STATE_WAIT_SIGNAL) {}

    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const p) override;

   private:
    static constexpr size_t decimation = 1;
    static constexpr size_t ADC_RATE = 3072000;
    static constexpr size_t AUDIO_RATE = ADC_RATE / decimation;

    // States enum (using full uint8_t to avoid bit-field issues)
    enum state_t : uint8_t {
        STATE_WAIT_SIGNAL,
        STATE_VIS_START,
        STATE_VIS_BITS,
        STATE_SYNC,
        STATE_PIXELS
    };
    state_t state;

    // Status flags packed into a small struct (in-class-init to avoid ctor warnings)
    struct Status {
        uint8_t configured : 1;
        uint8_t vis_code_valid : 1;
        uint8_t vis_bit_counter : 4;  // 0-8
        uint8_t unused : 2;
    } status{};

    // Counter bytes (all reduced ranges) with sensible defaults
    uint8_t vis_code{0};
    uint8_t sample_count{0};
    uint8_t sync_count{0};
    uint8_t pixel_count{0};
    uint8_t line_count{0};  // Reduced from uint16_t
    uint8_t line_width{0};  // Reduced from uint16_t, max 320
    uint8_t samples_per_pixel{0};
    uint8_t sync_width{0};

    // (removed unused prev_angle and curr_freq members - detect_tone uses local state)
    
    // Tone detection using reduced fixed-point scaling (Q8.4 format)
    static constexpr uint16_t FREQ_TOLERANCE_FP = 50 << 4;  // 50Hz with 4-bit fraction
    static constexpr uint16_t VIS_FREQ_1100_FP = 1100 << 4;
    static constexpr uint16_t VIS_FREQ_1200_FP = 1200 << 4;
    static constexpr uint16_t VIS_FREQ_1300_FP = 1300 << 4;

    // Minimal buffer for detection
    static constexpr uint8_t FIFO_SIZE = 8;
    std::array<int8_t, FIFO_SIZE> baseband_buffer{};  // Zero-initialize
    uint8_t buffer_pos{0};
    
    // Safety limits - further reduced
    static constexpr uint16_t MAX_LINE_WIDTH = 320;
    static constexpr uint16_t MAX_SAMPLES_PER_PIXEL = 256; // Further reduced from 512
    static constexpr uint16_t MAX_SYNC_WIDTH = 512;        // Further reduced from 1024

    // Message handling (moved to optimize memory layout)
    SSTVLineMessage line_message{};
    RequestSignalMessage frame_sync_message{RequestSignalMessage::Signal::FrameSync};

    // Core processing functions
    float detect_tone(float) __attribute__((hot));
    void process_vis_bit(float) __attribute__((hot));
    void send_line_data();
    void reset_rx_state();

    BasebandThread baseband_thread{AUDIO_RATE, this, baseband::Direction::Receive};
};

#endif