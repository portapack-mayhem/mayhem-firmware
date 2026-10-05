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

#include "ch.h"

#include <algorithm>

namespace {

constexpr bool valid_decimation(
    const size_t decimation_factor,
    const size_t maximum_decimation) {
    return decimation_factor >= 2 &&
           decimation_factor <= maximum_decimation &&
           (decimation_factor & (decimation_factor - 1)) == 0;
}

}  // namespace

void FilteredSpectrumCollector::on_message(const Message* const message) {
    if (message->id == Message::ID::UpdateSpectrum) {
        update();
    }
    SpectrumCollector::on_message(message);
}

bool FilteredSpectrumCollector::start_capture(
    const size_t decimation_factor) {
    if (!valid_decimation(decimation_factor, maximum_decimation)) {
        capture_ready_ = false;
        return false;
    }

    capture_decimation_ = decimation_factor;
    capture_count_ = 0;
    capture_ready_ = false;
    return true;
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

    const size_t required_samples = fft_samples * capture_decimation_;
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
        fft_samples * capture_decimation_,
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

void AMFilteredSpectrumCollector::on_message(const Message* const message) {
    if (message->id == Message::ID::UpdateSpectrum) {
        update();
    }
    SpectrumCollector::on_message(message);
}

bool AMFilteredSpectrumCollector::start_capture(
    const size_t decimation_factor) {
    if (!valid_decimation(decimation_factor, maximum_decimation)) {
        return false;
    }

    chSysLock();
    if (capture_state_ != CaptureState::Idle) {
        chSysUnlock();
        return false;
    }

    capture_decimation_ = decimation_factor;
    capture_count_ = 0;
    capture_state_ = CaptureState::Capturing;
    chSysUnlock();
    return true;
}

bool AMFilteredSpectrumCollector::feed(
    const buffer_c16_t& channel,
    const int32_t filter_low_frequency,
    const int32_t filter_high_frequency,
    const int32_t filter_transition) {
    chSysLock();
    if (capture_state_ != CaptureState::Capturing) {
        chSysUnlock();
        return false;
    }
    chSysUnlock();

    set_filter(
        filter_low_frequency,
        filter_high_frequency,
        filter_transition);

    const size_t required_samples = fft_samples * capture_decimation_;
    const size_t copy_count = std::min(
        channel.count, required_samples - capture_count_);
    std::copy_n(
        channel.p,
        copy_count,
        capture_.begin() + capture_count_);
    capture_count_ += copy_count;

    if (capture_count_ == required_samples) {
        sampling_rate_ = channel.sampling_rate / capture_decimation_;
        const bool streaming = is_streaming();

        chSysLock();
        if (streaming) {
            capture_state_ = CaptureState::Pending;
        } else {
            capture_state_ = CaptureState::Idle;
        }
        chSysUnlock();

        if (streaming) {
            EventDispatcher::events_flag(EVT_MASK_SPECTRUM);
        }
        return true;
    }
    return false;
}

void AMFilteredSpectrumCollector::update() {
    chSysLock();
    if (capture_state_ != CaptureState::Pending) {
        chSysUnlock();
        return;
    }
    capture_state_ = CaptureState::Processing;
    const size_t capture_decimation = capture_decimation_;
    const uint32_t sampling_rate = sampling_rate_;
    chSysUnlock();

    size_t filtered_count = fft_samples * capture_decimation;
    uint32_t filtered_sampling_rate = sampling_rate * capture_decimation;
    size_t remaining_decimation = capture_decimation;

    if (remaining_decimation == maximum_decimation) {
        decimator_4_.configure(taps_audio_spectrum_decim_4.taps);
        const buffer_c16_t input{
            capture_.data(),
            filtered_count,
            filtered_sampling_rate};
        const buffer_c16_t output{
            capture_.data(),
            filtered_count / decimator_4_.decimation_factor};
        const auto filtered = decimator_4_.execute(input, output);
        filtered_count = filtered.count;
        filtered_sampling_rate = filtered.sampling_rate;
        remaining_decimation /= decimator_4_.decimation_factor;
    }

    for (; remaining_decimation > 1; remaining_decimation /= 2) {
        decimator_.configure(taps_audio_spectrum_halfband.taps);
        const buffer_c16_t input{
            capture_.data(),
            filtered_count,
            filtered_sampling_rate};
        const buffer_c16_t output{
            capture_.data(),
            filtered_count / 2};
        const auto filtered = decimator_.execute(input, output);
        filtered_count = filtered.count;
        filtered_sampling_rate = filtered.sampling_rate;
    }

    post_message({capture_.data(), filtered_count, filtered_sampling_rate});
    chSysLock();
    capture_state_ = CaptureState::Idle;
    chSysUnlock();
}
