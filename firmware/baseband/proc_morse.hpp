/*
 * Copyright (C) 2026 Pezsma
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

#ifndef __PROC_MORSE_H__
#define __PROC_MORSE_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "message.hpp"
#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "audio_output.hpp"
#include "dsp_fir_taps.hpp"
#include "rssi_thread.hpp"

class MorseProcessor : public BasebandProcessor {
   public:
    MorseProcessor() {}

    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const p) override;
    void update_goertzel_coeff(float freq);

   private:
    bool configured{false};

    size_t baseband_fs = 3072000;

    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};

    std::array<complex16_t, 512> dst_buffer{};
    std::array<int16_t, 32> audio_buffer{};

    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};
    dsp::decimate::FIRC16xR16x32Decim8 decim_1{};
    dsp::decimate::FIRAndDecimateComplex channel_filter{};
    dsp::demodulate::FM demod{};
    AudioOutput audio_output{};

    // Goertzel and signal detection
    int32_t coeff_int{0};
    int32_t s_prev_i{0};
    int32_t s_prev2_i{0};
    uint32_t goertzel_count{0};
    uint32_t duration_samples{0};
    bool was_signaling{false};

    // Variables required for measurement
    int16_t prev_sample{0};
    uint32_t zc_counter{0};
    float current_freq{0.0f};

    int32_t lpf_sample{0};
    int32_t dc_offset{0};

    // Squelch and stability
    bool squelch_is_open{false};
    int32_t squelch_hold{0};

    int32_t user_squelch_level{0};
    int64_t noise_floor{0};

    int32_t startup_delay{0};    // Power-on delay
    int32_t last_zc_counter{0};  // Previous ZC measurement for comparison

    MorseRXDataMessage message{};

    void configure();
};

#endif  // __PROC_MORSE_H__