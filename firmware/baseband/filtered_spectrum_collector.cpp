/*
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#include "filtered_spectrum_collector.hpp"

#include "dsp_fir_taps.hpp"
#include "event_m4.hpp"

#include <algorithm>

void FilteredSpectrumCollector::on_message(const Message* const message) {
    if (message->id == Message::ID::UpdateSpectrum) {
        update();
    }
    SpectrumCollector::on_message(message);
}

void FilteredSpectrumCollector::start_capture(
    const size_t decimation_factor) {
    capture_decimation_ = decimation_factor;
    capture_count_ = 0;
    capture_ready_ = false;
}

bool FilteredSpectrumCollector::feed(
    const buffer_c16_t& channel,
    const int32_t filter_low_frequency,
    const int32_t filter_high_frequency,
    const int32_t filter_transition) {
    set_filter(
        filter_low_frequency,
        filter_high_frequency,
        filter_transition);

    const size_t required_samples = 256 * capture_decimation_;
    const size_t copy_count = std::min(
        channel.count, required_samples - capture_count_);
    std::copy_n(
        channel.p,
        copy_count,
        capture_.begin() + capture_count_);
    capture_count_ += copy_count;

    if (capture_count_ == required_samples) {
        sampling_rate_ = channel.sampling_rate / capture_decimation_;
        capture_ready_ = true;
        EventDispatcher::events_flag(EVT_MASK_SPECTRUM);
        if (is_streaming()) {
            capture_ready_ = true;
            EventDispatcher::events_flag(EVT_MASK_SPECTRUM);
        } else {
            capture_ready_ = false;
        }
        return true;
    }
    return false;
}

void FilteredSpectrumCollector::update() {
    if (!capture_ready_) {
        return;
    }

    decim_0_.configure(taps_audio_spectrum_halfband.taps);
    const buffer_c16_t capture{
        capture_.data(),
        256 * capture_decimation_,
        sampling_rate_ * capture_decimation_};
    const buffer_c16_t stage_0{
        stage_0_.data(),
        stage_0_.size()};
    const auto filtered = decim_0_.execute(capture, stage_0);

    if (capture_decimation_ == 4) {
        decim_1_.configure(taps_audio_spectrum_halfband.taps);
        const buffer_c16_t stage_1{
            stage_1_.data(),
            stage_1_.size()};
        post_message(decim_1_.execute(filtered, stage_1));
    } else {
        post_message(filtered);
    }
    capture_ready_ = false;
}
