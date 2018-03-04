/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "audio_output.hpp"

#include "portapack_shared_memory.hpp"

#include "audio_dma.hpp"

#include "message.hpp"

#include <cstdint>
#include <cstddef>
#include <array>

void AudioOutput::configure(
	const bool do_proc
) {
	do_processing = do_proc;
}

void AudioOutput::configure(
	const iir_biquad_config_t& hpf_config,
	const iir_biquad_config_t& deemph_config,
	const float squelch_threshold
) {
	hpf.configure(hpf_config);
	deemph.configure(deemph_config);
	squelch.set_threshold(squelch_threshold);
}

void AudioOutput::write(
	const buffer_s16_t& audio
) {
	std::array<float, 32> audio_f;
	for(size_t i=0; i<audio.count; i++) {
		audio_f[i] = audio.p[i] * ki;
	}
	write(buffer_f32_t {
		audio_f.data(),
		audio.count,
		audio.sampling_rate
	});
}

void AudioOutput::write(
	const buffer_f32_t& audio
) {
	block_buffer.feed(
		audio,
		[this](const buffer_f32_t& buffer) {
			this->on_block(buffer);
		}
	);
}

void AudioOutput::on_block(
	const buffer_f32_t& audio
) {
	if (do_processing) {
		const auto audio_present_now = squelch.execute(audio);

		hpf.execute_in_place(audio);
		deemph.execute_in_place(audio);

		audio_present_history = (audio_present_history << 1) | (audio_present_now ? 1 : 0);
		audio_present = (audio_present_history != 0);
		
		if( !audio_present ) {
			for(size_t i=0; i<audio.count; i++) {
				audio.p[i] = 0;
			}
		}
	} else
		audio_present = true;

	fill_audio_buffer(audio, audio_present);
}

bool AudioOutput::is_squelched() {
	return !audio_present;
}

void AudioOutput::fill_audio_buffer(const buffer_f32_t& audio, const bool send_to_fifo) {
	std::array<int16_t, 32> audio_int;

	auto audio_buffer = audio::dma::tx_empty_buffer();
	for(size_t i=0; i<audio_buffer.count; i++) {
		const int32_t sample_int = audio.p[i] * k;
		const int32_t sample_saturated = __SSAT(sample_int, 16);
		audio_buffer.p[i].left = audio_buffer.p[i].right = sample_saturated;
		audio_int[i] = sample_saturated;
	}
	if( stream && send_to_fifo ) {
		stream->write(audio_int.data(), audio_buffer.count * sizeof(audio_int[0]));
	}

	feed_audio_stats(audio);
}

void AudioOutput::feed_audio_stats(const buffer_f32_t& audio) {
	audio_stats.feed(
		audio,
		[](const AudioStatistics& statistics) {
			const AudioStatisticsMessage audio_stats_message { statistics };
			shared_memory.application_queue.push(audio_stats_message);
		}
	);
}
