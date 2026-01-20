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
    BasebandThread baseband_thread{3072000, this, baseband::Direction::Receive};
    void configure(uint8_t mode);
    buffer_f32_t demodulate(const buffer_c16_t& channel);

    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};
    dsp::decimate::FIRC16xR16x32Decim8 decim_1{};
    dsp::decimate::FIRAndDecimateComplex channel_filter{};
    dsp::demodulate::FM demod_cw_fm{};
    dsp::demodulate::SSB demod_ssb{};
    AudioOutput audio_output{};

    std::array<complex16_t, 512> dst{};
    const buffer_c16_t dst_buffer{
        dst.data(),
        dst.size()};
    std::array<float, 32> audio{};
    const buffer_f32_t audio_buffer{
        audio.data(),
        audio.size()};

    bool configured{false};
    bool was_signaling{false};    // Goertzel
    bool squelch_is_open{false};  // Squelch and stability
    uint8_t modulation{0};        // 0=CW/FM, 1=USB, 2=LSB

    int16_t prev_sample{0};  // Variables required for measurement

    uint32_t zc_counter{0};        // Variables required for measurement
    uint32_t goertzel_count{0};    // Goertzel
    uint32_t duration_samples{0};  // Goertzel

    int32_t coeff_int{0};  // Goertzel
    int32_t s_prev_i{0};   // Goertzel
    int32_t s_prev2_i{0};  // Goertzel
    int32_t lpf_sample{0};
    int32_t dc_offset{0};
    int32_t squelch_hold{0};        // Squelch and stability
    int32_t user_squelch_level{0};  // Squelch and stability
    int32_t startup_delay{0};       // Squelch and stability
    int32_t last_zc_counter{0};     // Squelch and stability
    int64_t noise_floor{0};         // Squelch and stability

    float current_freq{0.0f};  // Variables required for measurement

    // UI és időzítés
    int32_t ui_update_timer = 0;
    int32_t freq_hold_timer = 0;
    float display_freq = 0.0f;

    // Precíziós mérés változói
    int32_t samples_in_period = 0;      // Számláló egy teljes periódushoz
    bool signal_state_high = false;     // Schmitt-trigger állapota
    float freq_avg_accumulator = 0.0f;  // Átlagoláshoz
    int32_t freq_avg_count = 0;         // Átlagoláshoz

    MorseRXDataMessage message{};
    RSSIThread rssi_thread{};
};

#endif  // __PROC_MORSE_H__