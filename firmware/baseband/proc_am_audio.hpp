/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __PROC_AM_AUDIO_H__
#define __PROC_AM_AUDIO_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "audio_compressor.hpp"

#include "audio_output.hpp"
#include "spectrum_collector.hpp"

#include <cstdint>
#include <cmath>

class NarrowbandAMAudio : public BasebandProcessor {
   public:
    NarrowbandAMAudio();  // Phase 2: Explicit constructor for manual thread start

    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    static constexpr size_t baseband_fs = 3072000;
    static constexpr size_t decim_2_decimation_factor = 4;
    static constexpr size_t channel_filter_decimation_factor = 1;

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
    dsp::decimate::FIRAndDecimateComplex decim_2{};
    dsp::decimate::FIRAndDecimateComplex channel_filter{};
    int32_t channel_filter_low_f = 0;
    int32_t channel_filter_high_f = 0;
    int32_t channel_filter_transition = 0;
    bool configured{false};
    bool vor_enabled{false};
    static constexpr float vor_reference_hz{30.0f};
    static constexpr float vor_subcarrier_hz{9960.0f};
    static constexpr uint32_t vor_window_samples{4800};  // 100 ms at 48 kHz

    struct PhaseOscillator {
        float sin_v{0.0f};
        float cos_v{1.0f};
        float step_sin{0.0f};
        float step_cos{1.0f};

        void configure(float frequency_hz, float sample_rate_hz);
        float cosine() const { return cos_v; }
        float sine() const { return sin_v; }
        void advance();
    } vor_reference_osc{}, vor_subcarrier_osc{};
    float vor_ref_i{0.0f};
    float vor_ref_q{0.0f};
    float vor_var_i{0.0f};
    float vor_var_q{0.0f};
    uint32_t vor_sample_count{0};

    // bool modulation_ssb = false;  // Origianlly we only had 2 AM demod types {DSB = 0, SSB = 1} , and we could handle it with bool var , 1 bit.
    int8_t modulation_ssb = 0;  // Now we have 3 AM demod types we will send now index integer  {DSB = 0, SSB = 1, SSB_FM = 2}
    dsp::demodulate::AM demod_am{};
    dsp::demodulate::SSB demod_ssb{};
    dsp::demodulate::SSB_FM demod_ssb_fm{};  // added for Wfax mode.
    FeedForwardCompressor audio_compressor{};
    AudioOutput audio_output{};

    SpectrumCollector channel_spectrum{};

    /* NB: Threads should be the last members in the class definition. */
#ifdef PRALINE
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive,
                                   /*auto_start*/ false};  // Phase 2: Manual start
    RSSIThread rssi_thread{/*auto_start*/ false};          // Phase 2: Manual start
#else
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};
#endif

    void configure(const AMConfigureMessage& message);
    void configure_vor(const VorRxConfigureMessage& message);
    void capture_config(const CaptureConfigMessage& message);
    void process_vor_metrics(const buffer_f32_t& audio);
    void send_vor_status(uint16_t phase_deg, uint16_t radial_deg, uint16_t ref_level, uint16_t var_level, uint8_t quality, bool valid, bool to_from);
    static uint16_t normalize_degrees(float degrees);

    buffer_f32_t demodulate(const buffer_c16_t& channel);
};

#endif /*__PROC_AM_AUDIO_H__*/
