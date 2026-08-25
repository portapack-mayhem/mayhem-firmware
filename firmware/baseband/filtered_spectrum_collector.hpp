/*
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#ifndef __FILTERED_SPECTRUM_COLLECTOR_H__
#define __FILTERED_SPECTRUM_COLLECTOR_H__

#include "dsp_decimate.hpp"
#include "spectrum_collector.hpp"

#include <array>

class FilteredSpectrumCollector : public SpectrumCollector {
   public:
    void on_message(const Message* const message);

    bool start_capture(const size_t decimation_factor);
    bool feed(
        const buffer_c16_t& channel,
        const int32_t filter_low_frequency,
        const int32_t filter_high_frequency,
        const int32_t filter_transition);

   private:
    static constexpr size_t fft_samples = 256;
    static constexpr size_t maximum_decimation = 4;

    std::array<complex16_t, 1024> capture_{};
    std::array<complex16_t, 512> stage_0_{};
    std::array<complex16_t, 256> stage_1_{};
    dsp::decimate::FIRC16xR16x63HalfbandDecim2 decim_0_{};
    dsp::decimate::FIRC16xR16x63HalfbandDecim2 decim_1_{};
    size_t capture_count_{0};
    size_t capture_decimation_{1};
    uint32_t sampling_rate_{0};
    bool capture_ready_{false};

    void update();
};

class AMFilteredSpectrumCollector : public SpectrumCollector {
   public:
    void on_message(const Message* const message);

    bool start_capture(const size_t decimation_factor);
    bool feed(
        const buffer_c16_t& channel,
        const int32_t filter_low_frequency,
        const int32_t filter_high_frequency,
        const int32_t filter_transition);

   private:
    static constexpr size_t fft_samples = 256;
    static constexpr size_t maximum_decimation = 16;

    std::array<complex16_t, fft_samples * maximum_decimation> capture_{};
    dsp::decimate::FIRC16xR16x63HalfbandDecim2 decimator_{};
    size_t capture_count_{0};
    size_t capture_decimation_{1};
    uint32_t sampling_rate_{0};
    bool capture_ready_{false};

    void update();
};

#endif /*__FILTERED_SPECTRUM_COLLECTOR_H__*/
