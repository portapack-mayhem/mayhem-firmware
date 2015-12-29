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
	const uint32_t filter_pass_frequency,
	const uint32_t filter_stop_frequency
) {
	// Called from baseband processing thread.
	channel_filter_pass_frequency = filter_pass_frequency;
	channel_filter_stop_frequency = filter_stop_frequency;
	channel_spectrum_decimator.feed(
		channel,
		[this](const buffer_c16_t& data) {
			this->post_message(data);
		}
	);
}

void SpectrumCollector::post_message(const buffer_c16_t& data) {
	// Called from baseband processing thread.
	if( !channel_spectrum_request_update ) {
		fft_swap(data, channel_spectrum);
		channel_spectrum_sampling_rate = data.sampling_rate;
		channel_spectrum_request_update = true;
		events_flag(EVT_MASK_SPECTRUM);
	}
}

void SpectrumCollector::update() {
	// Called from idle thread (after EVT_MASK_SPECTRUM is flagged)
	if( channel_spectrum_request_update ) {
		/* Decimated buffer is full. Compute spectrum. */
		channel_spectrum_request_update = false;
		fft_c_preswapped(channel_spectrum);

		ChannelSpectrumMessage spectrum_message;
		for(size_t i=0; i<spectrum_message.spectrum.db.size(); i++) {
			// Three point Hamming window.
			const auto corrected_sample = channel_spectrum[i] * 0.54f
				+ (channel_spectrum[(i-1) & 0xff] + channel_spectrum[(i+1) & 0xff]) * -0.23f;
			const auto mag2 = magnitude_squared(corrected_sample);
			const float db = complex16_mag_squared_to_dbv_norm(mag2);
			constexpr float mag_scale = 5.0f;
			const unsigned int v = (db * mag_scale) + 255.0f;
			spectrum_message.spectrum.db[i] = std::max(0U, std::min(255U, v));
		}

		/* TODO: Rename .db -> .magnitude, or something more (less!) accurate. */
		spectrum_message.spectrum.db_count = spectrum_message.spectrum.db.size();
		spectrum_message.spectrum.sampling_rate = channel_spectrum_sampling_rate;
		spectrum_message.spectrum.channel_filter_pass_frequency = channel_filter_pass_frequency;
		spectrum_message.spectrum.channel_filter_stop_frequency = channel_filter_stop_frequency;
		shared_memory.application_queue.push(spectrum_message);
	}
}
