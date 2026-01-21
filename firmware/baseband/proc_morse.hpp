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

   private:
    void configure(uint8_t mode);
    buffer_f32_t demodulate(const buffer_c16_t& channel);
    void set_decoder_freq(float freq);
    void measure_frequency(int32_t sample);

    BasebandThread baseband_thread{3072000, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};

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
    dsp::demodulate::FM demod_cw_fm{};
    dsp::demodulate::SSB demod_ssb{};
    AudioOutput audio_output{};

    bool configured{false};

    uint8_t modulation{0};  // 0=CW/FM, 1=USB, 2=LSB
    bool squelch_is_open{false};
    int32_t user_squelch_level{0};

    // --- 1. CSOPORT: DEKÓDOLÁS VÁLTOZÓI (goog_decode logika) ---
    // Ezeket a mérő rutin NEM módosíthatja!
    int32_t dec_coeff_int{0};  // Goertzel koefficiens
    int32_t dec_s_prev_i{0};
    int32_t dec_s_prev2_i{0};
    uint32_t dec_goertzel_count{0};
    uint32_t dec_duration_samples{0};
    bool dec_was_signaling{false};
    int64_t dec_noise_floor{0};

    // --- 2. CSOPORT: FREKVENCIA MÉRÉS VÁLTOZÓI (good_fequency_meas logika) ---
    // Ezek csak a kijelzőt (measured_frequency) frissítik
    int16_t meas_prev_sample{0};
    uint32_t meas_zc_counter{0};
    uint32_t meas_samples_in_period{0};
    bool meas_signal_state_high{false};
    float meas_avg_accumulator{0.0f};
    uint32_t meas_avg_count{0};
    uint32_t meas_last_period_len{0};
    uint32_t meas_consistency_count{0};

    int32_t meas_lpf{0};          // Aluláteresztő szűrő tároló
    bool meas_state_high{false};  // Hiszterézis állapot (Fent/Lent)
    uint32_t ui_update_timer{0};

    float meas_freq_accumulator{0.0f};
    uint32_t meas_freq_count{0};

    // zajméréshez
    int32_t trigger_level{500};

    MorseRXDataMessage message{};
};

#endif