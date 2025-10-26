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

#ifndef __PROC_SSTV_RX__
#define __PROC_SSTV_RX__

#include "portapack_shared_memory.hpp"
#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "sstv.hpp"

using namespace sstv;

class SSTVRXProcessor : public BasebandProcessor {
    public:
     void execute(const buffer_c8_t& buffer) override;
     void on_message(const Message* const p) override;

    private:
     enum state_t {
        STATE_SYNC_SEARCH = 0,
        STATE_VIS_DECODE,
        STATE_IMAGE_DATA
     };

     static constexpr uint32_t MAX_SAMPLES_PER_LINE = 4096;
     static constexpr uint16_t PIXELS_PER_LINE = 320;
     
     // Frequency ranges for SSTV (in Hz)
     static constexpr int32_t FREQ_BLACK = 1500;
     static constexpr int32_t FREQ_WHITE = 2300;
     static constexpr int32_t FREQ_SYNC = 1200;
     static constexpr int32_t FREQ_VIS_BIT0 = 1300;
     static constexpr int32_t FREQ_VIS_BIT1 = 1100;
     
     state_t state{STATE_SYNC_SEARCH};
     bool configured{false};
     uint8_t vis_code{0};
     
     // FM demodulation state
     int32_t prev_i{0};
     int32_t prev_q{0};
     
     // Frequency estimation (using zero-crossing method)
     int32_t prev_sample{0};
     uint32_t zero_cross_timer{0};
     uint32_t zero_cross_count{0};
     int32_t current_freq{0};  // Current estimated frequency in Hz
     
     // Line decoding state
     uint8_t line_buffer_r[PIXELS_PER_LINE];
     uint8_t line_buffer_g[PIXELS_PER_LINE];
     uint8_t line_buffer_b[PIXELS_PER_LINE];
     
     uint32_t sample_count{0};
     uint32_t pixel_index{0};
     uint32_t channel_index{0};  // 0=G, 1=B, 2=R for Scottie
     uint16_t current_line{0};
     
     // Timing parameters (will be set based on mode)
     uint32_t samples_per_pixel{846};  // Scottie 2: 0.2752ms at 3.072MHz
     uint32_t samples_per_sync{27648};  // 9ms
     uint32_t samples_per_gap{4608};    // 1.5ms
     
     // Sync detection
     uint32_t sync_sample_count{0};
     bool in_sync{false};
     
     // Helper functions
     int32_t freq_to_pixel(int32_t freq);
     void process_pixel_sample(int32_t freq);
     void process_line();
     void detect_sync(int32_t freq);
     void estimate_frequency(int32_t demod_sample);

     RequestSignalMessage sig_message{RequestSignalMessage::Signal::FillRequest};

     /* NB: Threads should be the last members in the class definition. */
     BasebandThread baseband_thread{307200, this, baseband::Direction::Receive};
};

#endif