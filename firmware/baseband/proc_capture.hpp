/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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

#ifndef __PROC_CAPTURE_HPP__
#define __PROC_CAPTURE_HPP__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "dsp_decimate.hpp"
#include "spectrum_collector.hpp"
#include "stream_input.hpp"

#include <array>
#include <memory>

class CaptureProcessor : public BasebandProcessor {
   public:
    CaptureProcessor();

    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    size_t baseband_fs = 3072000;  // aka: sample_rate
    static constexpr auto spectrum_rate_hz = 50.0f;

    std::array<complex16_t, 512> dst{};
    const buffer_c16_t dst_buffer{
        dst.data(),
        dst.size()};

    /* NB: There are two decimation passes: 0 and 1. In pass 0, one of
     * the following will be selected based on the oversample rate.
     * use decim_0_4 for an overall decimation factor of 8.
     * use decim_0_8 for an overall decimation factor of 16. */
    dsp::decimate::FIRC8xR16x24FS4Decim4 decim_0_4{};
    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0_8{};
    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0_8_180k{};
    dsp::decimate::FIRC16xR16x16Decim2 decim_1{};

    int32_t channel_filter_low_f = 0;
    int32_t channel_filter_high_f = 0;
    int32_t channel_filter_transition = 0;

    std::unique_ptr<StreamInput> stream{};

    SpectrumCollector channel_spectrum{};
    size_t spectrum_interval_samples = 0;
    size_t spectrum_samples = 0;
    OversampleRate oversample_rate{OversampleRate::x8};

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{
        baseband_fs, this, baseband::Direction::Receive, /*auto_start*/ false};
    RSSIThread rssi_thread{};

    void sample_rate_config(const SampleRateConfigMessage& message);
    void capture_config(const CaptureConfigMessage& message);

    /* Dispatch to the correct decim_0 based on oversample rate. */
    buffer_c16_t decim_0_execute(const buffer_c8_t& src, const buffer_c16_t& dst);
};

#endif /*__PROC_CAPTURE_HPP__*/
