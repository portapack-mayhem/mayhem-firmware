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

#include "spectrum_collector.hpp"

#include "dsp_fft.hpp"
#include "dsp_fir_taps.hpp"

#include "utility.hpp"
#include "event_m4.hpp"
#include "portapack_shared_memory.hpp"

#include <algorithm>

void SpectrumCollector::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::UpdateSpectrum:
            update();
            break;

        case Message::ID::SpectrumStreamingConfig:
            set_state(*reinterpret_cast<const SpectrumStreamingConfigMessage*>(message));
            break;

        default:
            break;
    }
}

void SpectrumCollector::set_state(const SpectrumStreamingConfigMessage& message) {
    if (message.mode == SpectrumStreamingConfigMessage::Mode::Running) {
        start();
    } else {
        stop();
    }
}

void SpectrumCollector::start() {
    streaming = true;
    ChannelSpectrumConfigMessage message{&fifo};
    shared_memory.application_queue.push(message);
}

void SpectrumCollector::stop() {
    streaming = false;
    fifo.reset_in();
}

void SpectrumCollector::set_decimation_factor(
    const size_t decimation_factor) {
    channel_spectrum_decimator.set_factor(decimation_factor);
}

/* TODO: Refactor to register task with idle thread?
 * It's sad that the idle thread has to call all the way back here just to
 * perform the deferred task on the buffer of data we prepared.
 */

bool SpectrumCollector::feed(
    const buffer_c16_t& channel,
    const int32_t filter_low_frequency,
    const int32_t filter_high_frequency,
    const int32_t filter_transition) {
    // Called from baseband processing thread.
    channel_filter_low_frequency = filter_low_frequency;
    channel_filter_high_frequency = filter_high_frequency;
    channel_filter_transition = filter_transition;

    bool block_completed = false;
    channel_spectrum_decimator.feed(
        channel,
        [this, &block_completed](const buffer_c16_t& data) {
            this->post_message(data);
            block_completed = true;
        });
    return block_completed;
}

void SpectrumCollector::start_filtered_capture(
    const size_t decimation_factor) {
    filtered_capture_decimation_ = decimation_factor;
    filtered_capture_count_ = 0;
    filtered_capture_ready_ = false;
}

bool SpectrumCollector::feed_filtered(
    const buffer_c16_t& channel,
    const int32_t filter_low_frequency,
    const int32_t filter_high_frequency,
    const int32_t filter_transition) {
    channel_filter_low_frequency = filter_low_frequency;
    channel_filter_high_frequency = filter_high_frequency;
    channel_filter_transition = filter_transition;

    const size_t required_samples = 256 * filtered_capture_decimation_;
    const size_t copy_count = std::min(
        channel.count, required_samples - filtered_capture_count_);
    std::copy_n(
        channel.p,
        copy_count,
        filtered_capture_.begin() + filtered_capture_count_);
    filtered_capture_count_ += copy_count;

    if (filtered_capture_count_ == required_samples) {
        channel_spectrum_sampling_rate =
            channel.sampling_rate / filtered_capture_decimation_;
        filtered_capture_ready_ = true;
        channel_spectrum_request_update = true;
        EventDispatcher::events_flag(EVT_MASK_SPECTRUM);
        return true;
    }
    return false;
}

void SpectrumCollector::post_message(const buffer_c16_t& data) {
    // Called from baseband processing thread.
    if (streaming && !channel_spectrum_request_update) {
        fft_swap(data, channel_spectrum);
        channel_spectrum_sampling_rate = data.sampling_rate;
        channel_spectrum_request_update = true;
        EventDispatcher::events_flag(EVT_MASK_SPECTRUM);
    }
}

template <typename T>
static typename T::value_type spectrum_window_none(const T& s, const size_t i) {
    constexpr size_t length = sizeof(s) / sizeof(s[0]);
    static_assert(power_of_two(length), "Array length must be power of 2");
    return s[i];
};

template <typename T>
static typename T::value_type spectrum_window_hamming_3(const T& s, const size_t i) {
    constexpr size_t length = sizeof(s) / sizeof(s[0]);
    static_assert((length), "Array length must be power of 2");
    constexpr size_t mask = length - 1;
    // Three point Hamming window.
    return s[i] * 0.54f + (s[(i - 1) & mask] + s[(i + 1) & mask]) * -0.23f;
};

template <typename T>
static typename T::value_type spectrum_window_blackman_3(const T& s, const size_t i) {
    constexpr size_t length = sizeof(s) / sizeof(s[0]);
    static_assert(power_of_two(length), "Array length must be power of 2");
    constexpr size_t mask = length - 1;
    // Three term Blackman window.
    constexpr float alpha = 0.42f;
    constexpr float beta = 0.5f * 0.5f;
    constexpr float gamma = 0.08f * 0.05f;
    return s[i] * alpha - (s[(i - 1) & mask] + s[(i + 1) & mask]) * beta + (s[(i - 2) & mask] + s[(i + 2) & mask]) * gamma;
};

void SpectrumCollector::update() {
    // Called from idle thread (after EVT_MASK_SPECTRUM is flagged)
    if (streaming && channel_spectrum_request_update) {
        if (filtered_capture_ready_) {
            filtered_decim_0_.configure(taps_audio_spectrum_halfband.taps);
            const buffer_c16_t capture{
                filtered_capture_.data(),
                256 * filtered_capture_decimation_,
                channel_spectrum_sampling_rate * filtered_capture_decimation_};
            const buffer_c16_t stage_0{
                filtered_stage_0_.data(),
                filtered_stage_0_.size()};
            const auto filtered = filtered_decim_0_.execute(capture, stage_0);

            if (filtered_capture_decimation_ == 4) {
                filtered_decim_1_.configure(taps_audio_spectrum_halfband.taps);
                const buffer_c16_t stage_1{
                    filtered_stage_1_.data(),
                    filtered_stage_1_.size()};
                const auto zoom_filtered =
                    filtered_decim_1_.execute(filtered, stage_1);
                fft_swap(zoom_filtered, channel_spectrum);
            } else {
                fft_swap(filtered, channel_spectrum);
            }
            filtered_capture_ready_ = false;
        }

        /* Decimated buffer is full. Compute spectrum. */
        fft_c_preswapped(channel_spectrum, 0, 8);

        ChannelSpectrum spectrum;
        spectrum.sampling_rate = channel_spectrum_sampling_rate;
        spectrum.channel_filter_offset = channel_filter_offset;
        spectrum.channel_filter_low_frequency = channel_filter_low_frequency;
        spectrum.channel_filter_high_frequency = channel_filter_high_frequency;
        spectrum.channel_filter_transition = channel_filter_transition;
        for (size_t i = 0; i < spectrum.db.size(); i++) {
            const auto corrected_sample = spectrum_window_hamming_3(channel_spectrum, i);
            const auto mag2 = magnitude_squared(corrected_sample * (1.0f / 32768.0f));
            const float db = mag2_to_dbv_norm(mag2);
            constexpr float mag_scale = 5.0f;
            const unsigned int v = (db * mag_scale) + 255.0f;
            spectrum.db[i] = std::max(0U, std::min(255U, v));
        }
        fifo.in(spectrum);
    }

    channel_spectrum_request_update = false;
}
