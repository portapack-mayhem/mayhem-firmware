/*
 * Copyright (C) 2024 PortaPack Mayhem
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

#ifndef __PROC_TONEDETECT_H__
#define __PROC_TONEDETECT_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "message.hpp"
#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "audio_output.hpp"
#include "dsp_fir_taps.hpp"
#include "rssi_thread.hpp"
#include "spectrum_collector.hpp"
#include "dsp_squelch.hpp"

class ToneDetectProcessor : public BasebandProcessor {
   public:
    ToneDetectProcessor() {}

    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const p) override;

   private:
    void configure(uint8_t squelch, uint32_t ctcss_freq_x10);

    BasebandThread baseband_thread{3072000, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};

    std::array<complex16_t, 512> dst{};
    const buffer_c16_t dst_buffer{dst.data(), dst.size()};
    std::array<float, 32> audio{};
    const buffer_f32_t audio_buffer{audio.data(), audio.size()};

    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};
    dsp::decimate::FIRC16xR16x32Decim8 decim_1{};
    dsp::decimate::FIRAndDecimateComplex channel_filter{};
    dsp::demodulate::FM demod_fm{};
    AudioOutput audio_output{};

    bool configured{false};
    uint8_t user_squelch_level{0};
    FMSquelch fm_squelch{};  // user-adjustable: drives audio muting only
    FMSquelch carrier_sq{};  // fixed threshold: drives detection gate

    // Audio muting squelch state (user-adjustable level, has hold for smooth audio)
    bool squelch_is_open{false};
    uint32_t squelch_hold{0};

    // Carrier-detect gate state (fixed 0.20 threshold, separate from audio squelch)
    bool carrier_is_open{false};
    uint32_t carrier_hold{0};

    // CTCSS gate via Goertzel algorithm (per 40 ms window)
    uint32_t ctcss_freq_x10{0};  // 0 = None → fall back to FMSquelch gate
    float goertzel_coeff{0.0f};  // 2 * cos(2π * k / WINDOW_SAMPLES)
    float goertzel_s1{0.0f};
    float goertzel_s2{0.0f};

    // 40 ms measurement window
    uint32_t window_sample_count{0};
    bool was_ctcss_detected{false};
    uint32_t tone_duration_windows{0};

    // MOTO frequency bank — one Goertzel filter per MOTO table entry.
    // States reset each window; coefficients set once in configure().
    float moto_coeff[45]{};
    float moto_s1[45]{};
    float moto_s2[45]{};

    ToneDetectDataMessage data_message{};
    SpectrumCollector channel_spectrum{};
};

#endif /* __PROC_TONEDETECT_H__ */
