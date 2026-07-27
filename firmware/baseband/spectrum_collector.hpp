/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __SPECTRUM_COLLECTOR_H__
#define __SPECTRUM_COLLECTOR_H__

#define ARRAY_ELEMENTS(x) (sizeof(x) / sizeof(x[0]))
/* sizeof() compile-time operator that returns #bytes of (data type). We used it to get #elements_array */

#include "dsp_types.hpp"
#include "complex.hpp"

#include "block_decimator.hpp"
#include "dsp_decimate.hpp"

#include <cstdint>
#include <array>

#include "message.hpp"

class SpectrumCollector {
   public:
    void on_message(const Message* const message);

    void set_decimation_factor(const size_t decimation_factor);
    void set_channel_filter_offset(const int32_t offset) {
        channel_filter_offset = offset;
    }

    bool feed(
        const buffer_c16_t& channel,
        const int32_t filter_low_frequency,
        const int32_t filter_high_frequency,
        const int32_t filter_transition);

    void start_filtered_capture(const size_t decimation_factor);
    bool feed_filtered(
        const buffer_c16_t& channel,
        const int32_t filter_low_frequency,
        const int32_t filter_high_frequency,
        const int32_t filter_transition);

   private:
    BlockDecimator<complex16_t, 256> channel_spectrum_decimator{1};
    ChannelSpectrum fifo_data[1 << ChannelSpectrumConfigMessage::fifo_k]{};
    ChannelSpectrumFIFO fifo{fifo_data, ChannelSpectrumConfigMessage::fifo_k};

    volatile bool channel_spectrum_request_update{false};
    bool streaming{false};
    std::array<std::complex<float>, 256> channel_spectrum{};
    std::array<complex16_t, 1024> filtered_capture_{};
    std::array<complex16_t, 512> filtered_stage_0_{};
    std::array<complex16_t, 256> filtered_stage_1_{};
    dsp::decimate::FIRC16xR16x63HalfbandDecim2 filtered_decim_0_{};
    dsp::decimate::FIRC16xR16x63HalfbandDecim2 filtered_decim_1_{};
    size_t filtered_capture_count_{0};
    size_t filtered_capture_decimation_{1};
    bool filtered_capture_ready_{false};
    uint32_t channel_spectrum_sampling_rate{0};
    int32_t channel_filter_low_frequency{0};
    int32_t channel_filter_high_frequency{0};
    int32_t channel_filter_transition{0};
    int32_t channel_filter_offset{0};

    void post_message(const buffer_c16_t& data);

    void set_state(const SpectrumStreamingConfigMessage& message);
    void start();
    void stop();

    void update();
};

#endif /*__SPECTRUM_COLLECTOR_H__*/
