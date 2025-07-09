/*
 * Copyright (C) 2025 Brumi, HTotoo
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

#ifndef __PROC_NOAAAPTRX_H__
#define __PROC_NOAAAPTRX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "dsp_iir.hpp"
#include "audio_compressor.hpp"

#include "audio_output.hpp"
#include "spectrum_collector.hpp"

#include <cstdint>

class NoaaAptRx : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    void update_params();
    // todo rethink

    uint32_t samples_per_pixel = 0;

    // to exactly match the pixel / samples.
    double pxRem = 0;   // if has remainder, it'll store it
    double pxRoll = 0;  // summs remainders, so won't misalign

    uint32_t cnt = 0;  // signal counter
    uint16_t sync_cnt = 0;
    uint16_t syncnot_cnt = 0;

    static constexpr size_t baseband_fs = 3072000;
    static constexpr auto spectrum_rate_hz = 50.0f;

    std::array<complex16_t, 512> dst{};
    const buffer_c16_t dst_buffer{
        dst.data(),
        dst.size()};
    // work_audio_buffer and dst_buffer use the same data pointer
    const buffer_s16_t work_audio_buffer{
        (int16_t*)dst.data(),
        sizeof(dst) / sizeof(int16_t)};

    std::array<complex16_t, 64> complex_audio{};
    const buffer_c16_t complex_audio_buffer{
        complex_audio.data(),
        complex_audio.size()};

    dsp::decimate::FIRC8xR16x24FS4Decim4 decim_0{};
    // dsp::decimate::FIRC16xR16x16Decim2 decim_1{};   //original condition , before adding wfmam

    // decim_1 will handle different types of FIR filters depending on selection.
    dsp::decimate::FIRC16xR16x32Decim8 decim_1{};

    // dsp::decimate::FIRC16xR16x32Decim8 decim_1{}; // For FMAM

    int32_t channel_filter_low_f = 0;
    int32_t channel_filter_high_f = 0;
    int32_t channel_filter_transition = 0;

    dsp::demodulate::FM demod{};
    dsp::decimate::DecimateBy2CIC4Real audio_dec_1{};
    dsp::decimate::DecimateBy2CIC4Real audio_dec_2{};
    dsp::decimate::FIR64AndDecimateBy2Real audio_filter{};

    AudioOutput audio_output{};

    // For fs=96kHz FFT streaming
    BlockDecimator<complex16_t, 256> audio_spectrum_decimator{1};
    std::array<std::complex<float>, 256> audio_spectrum{};
    uint32_t audio_spectrum_timer{0};
    enum AudioSpectrumState {
        IDLE = 0,
        FEED,
        FFT
    };
    AudioSpectrumState audio_spectrum_state{IDLE};
    AudioSpectrum spectrum{};
    uint32_t fft_step{0};

    // SpectrumCollector channel_spectrum{};
    // size_t spectrum_interval_samples = 0;
    // size_t spectrum_samples = 0;

    bool configured{false};

    void configure(const NoaaAptRxConfigureMessage& message);
    void capture_config(const CaptureConfigMessage& message);

    NoaaAptRxStatusDataMessage status_message{0};
    NoaaAptRxImageDataMessage image_message{};

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};
};

#endif /*__PROC_NOAAAPTRX_H__*/
