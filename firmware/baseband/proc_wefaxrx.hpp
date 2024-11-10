/*
 * Copyright (C) 2024 HTotoo
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

#ifndef __PROC_WEFAXRX_H__
#define __PROC_WEFAXRX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "dsp_iir.hpp"

#include "audio_output.hpp"
#include "spectrum_collector.hpp"

#include <cstdint>

class WeFaxRx : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    void configure(const WeFaxRxConfigureMessage& message);
    void update_params();
    double calculateFrequencyDeviation(complex16_t& iq, complex16_t& iqlast);
    double calculatePhaseAngle(int16_t i, int16_t q);
    // todo rethink
    uint8_t lpm = 120;     // 60, 90, 100, 120, 180, 240 lpm
    uint8_t ioc_mode = 0;  // 0 - ioc576, 1 - ioc 288, 2 - colour fax
    uint32_t time_per_pixel = 595;
    uint32_t samples_per_pixel = 0;
    uint32_t time_start_tone = 3000;  // 3s - 5s
    uint32_t freq_start_tone = 300;   // 300hz for ioc576 675hz for ioc288, 200hz for colour fax
    uint32_t freq_stop_tone = 450;    // 450hz for the 3-5s stop tone
    uint16_t max_lines = 2500;        // to auto stop when too big
    uint16_t curr_lines = 0;          // current line number
    uint16_t freq_px_max = 2300;      // white
    uint16_t freq_px_min = 1500;      // black

    double pxRem = 0;   // if has remainder, it'll store it
    double pxRoll = 0;  // summs remainders, so won't misalign
    // float last_sig = 0;
    uint32_t wcnt = 0;
    uint32_t cnt = 0;

    static constexpr size_t baseband_fs = 3072000;
    size_t decim_0_input_fs = 0;
    size_t decim_0_output_fs = 0;
    size_t decim_1_input_fs = 0;
    size_t decim_1_output_fs = 0;
    size_t decim_2_input_fs = 0;
    size_t decim_2_output_fs = 0;
    size_t channel_filter_input_fs = 0;
    size_t channel_filter_output_fs = 0;

    bool configured{false};

    complex16_t iqlast = 0;

    std::array<complex16_t, 512> dst{};
    const buffer_c16_t dst_buffer{
        dst.data(),
        dst.size()};
    std::array<float, 32> audio{};
    const buffer_f32_t audio_buffer{
        audio.data(),
        audio.size()};

    dsp::demodulate::FM demod{};
    AudioOutput audio_output{};
    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};
    dsp::decimate::FIRC16xR16x32Decim8 decim_1{};
    dsp::decimate::FIRAndDecimateComplex decim_2{};
    dsp::decimate::FIRAndDecimateComplex channel_filter{};

    enum State {
        WAIT_FLAG,
        START,
        PHASING,
        IMAGE,
        WAIT_STOP
    };

    WeFaxRxStatusDataMessage status_message{0, 0};
    WeFaxRxImageDataMessage image_message{};

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};
};

#endif /*__PROC_WEFAXRX_H__*/
