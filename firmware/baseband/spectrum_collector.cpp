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

#include "utility.hpp"
#include "event_m4.hpp"
#include "portapack_shared_memory.hpp"

#include <algorithm>

void SpectrumCollector::on_message(const Message* const message) {
	switch(message->id) {
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
	if( message.mode == SpectrumStreamingConfigMessage::Mode::Running ) {
		start();
	} else {
		stop();
	}
}

void SpectrumCollector::start() {
	streaming = true;
	ChannelSpectrumConfigMessage message { &fifo };
	shared_memory.application_queue.push(message);
}

void SpectrumCollector::stop() {
	streaming = false;
	fifo.reset_in();
}

void SpectrumCollector::set_decimation_factor(
	const size_t decimation_factor
) {
	channel_spectrum_decimator.set_factor(decimation_factor);
}

/* TODO: Refactor to register task with idle thread?
 * It's sad that the idle thread has to call all the way back here just to
 * perform the deferred task on the buffer of data we prepared.
 */

void SpectrumCollector::feed(
	const buffer_c16_t& channel,
	const int32_t filter_low_frequency,
	const int32_t filter_high_frequency,
	const int32_t filter_transition
) {
	// Called from baseband processing thread.
	channel_filter_low_frequency = filter_low_frequency;
	channel_filter_high_frequency = filter_high_frequency;
	channel_filter_transition = filter_transition;

	channel_spectrum_decimator.feed(
		channel,
		[this](const buffer_c16_t& data) {
			this->post_message(data);
		}
	);
}

void SpectrumCollector::post_message(const buffer_c16_t& data) {
	// Called from baseband processing thread.
	if( streaming && !channel_spectrum_request_update ) {
		fft_swap(data, channel_spectrum);
		channel_spectrum_sampling_rate = data.sampling_rate;
		channel_spectrum_request_update = true;
		EventDispatcher::events_flag(EVT_MASK_SPECTRUM);
	}
}

template<typename T>
static typename T::value_type spectrum_window_none(const T& s, const size_t i) {
static_assert(power_of_two(ARRAY_ELEMENTS(s)), "Array number of elements must be power of 2");   // c/m compile error GCC10 , OK for all GCC versions. 
	return s[i];
};

template<typename T>
static typename T::value_type spectrum_window_hamming_3(const T& s, const size_t i) {
    static_assert(power_of_two(ARRAY_ELEMENTS(s)), "Array number of elements must be power of 2");   // c/m compile error GCC10 , OK for all GCC versions. 
	const size_t mask = s.size() - 1;          // c/m compile error GCC10 , constexpr->const
	// Three point Hamming window.
	return s[i] * 0.54f + (s[(i-1) & mask] + s[(i+1) & mask]) * -0.23f;
};

template<typename T>
static typename T::value_type spectrum_window_blackman_3(const T& s, const size_t i) {
    static_assert(power_of_two(ARRAY_ELEMENTS(s)), "Array number of elements must be power of 2");   // c/m compile error GCC10 , OK for all GCC versions. 
    const size_t mask = s.size() - 1;          // c/m compile error GCC10 , constexpr->const
	// Three term Blackman window.
	constexpr float alpha = 0.42f;
	constexpr float beta = 0.5f * 0.5f;
	constexpr float gamma = 0.08f * 0.05f;
	return s[i] * alpha - (s[(i-1) & mask] + s[(i+1) & mask]) * beta + (s[(i-2) & mask] + s[(i+2) & mask]) * gamma;
};

void SpectrumCollector::update() {
	// Called from idle thread (after EVT_MASK_SPECTRUM is flagged)
	if( streaming && channel_spectrum_request_update ) {
		/* Decimated buffer is full. Compute spectrum. */
		fft_c_preswapped(channel_spectrum, 0, 8);

		ChannelSpectrum spectrum;
		spectrum.sampling_rate = channel_spectrum_sampling_rate;
		spectrum.channel_filter_low_frequency = channel_filter_low_frequency;
		spectrum.channel_filter_high_frequency = channel_filter_high_frequency;
		spectrum.channel_filter_transition = channel_filter_transition;
		for(size_t i=0; i<spectrum.db.size(); i++) {
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
