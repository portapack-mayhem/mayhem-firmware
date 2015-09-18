/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "baseband_processor.hpp"

#include "portapack_shared_memory.hpp"

#include "dsp_fft.hpp"

#include "audio_dma.hpp"

#include "message.hpp"
#include "event_m4.hpp"
#include "utility.hpp"

#include <cstddef>
#include <algorithm>

void BasebandProcessor::update_spectrum() {
	// Called from idle thread (after EVT_MASK_SPECTRUM is flagged)
	if( channel_spectrum_request_update ) {
		/* Decimated buffer is full. Compute spectrum. */
		channel_spectrum_request_update = false;
		fft_c_preswapped(channel_spectrum);

		ChannelSpectrumMessage spectrum_message;
		for(size_t i=0; i<spectrum_message.spectrum.db.size(); i++) {
			const auto mag2 = magnitude_squared(channel_spectrum[i]);
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

void BasebandProcessor::feed_channel_stats(const buffer_c16_t channel) {
	channel_stats.feed(
		channel,
		[this](const ChannelStatistics statistics) {
			this->post_channel_stats_message(statistics);
		}
	);
}

void BasebandProcessor::feed_channel_spectrum(
	const buffer_c16_t channel,
	const uint32_t filter_pass_frequency,
	const uint32_t filter_stop_frequency
) {
	channel_filter_pass_frequency = filter_pass_frequency;
	channel_filter_stop_frequency = filter_stop_frequency;
	channel_spectrum_decimator.feed(
		channel,
		[this](const buffer_c16_t data) {
			this->post_channel_spectrum_message(data);
		}
	);
}

void BasebandProcessor::fill_audio_buffer(const buffer_s16_t audio) {
	auto audio_buffer = audio::dma::tx_empty_buffer();;
	for(size_t i=0; i<audio_buffer.count; i++) {
		audio_buffer.p[i].left = audio_buffer.p[i].right = audio.p[i];
	}
	i2s::i2s0::tx_unmute();

	feed_audio_stats(audio);
}

void BasebandProcessor::post_channel_stats_message(const ChannelStatistics statistics) {
	channel_stats_message.statistics = statistics;
	shared_memory.application_queue.push(channel_stats_message);
}

void BasebandProcessor::post_channel_spectrum_message(const buffer_c16_t data) {
	if( !channel_spectrum_request_update ) {
		fft_swap(data, channel_spectrum);
		channel_spectrum_sampling_rate = data.sampling_rate;
		channel_spectrum_request_update = true;
		events_flag(EVT_MASK_SPECTRUM);
	}
}

void BasebandProcessor::feed_audio_stats(const buffer_s16_t audio) {
	audio_stats.feed(
		audio,
		[this](const AudioStatistics statistics) {
			this->post_audio_stats_message(statistics);
		}
	);
}

void BasebandProcessor::post_audio_stats_message(const AudioStatistics statistics) {
	audio_stats_message.statistics = statistics;
	shared_memory.application_queue.push(audio_stats_message);
}
