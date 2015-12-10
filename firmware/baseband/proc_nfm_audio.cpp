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

#include "proc_nfm_audio.hpp"

#include <cstdint>
#include <cstddef>

void NarrowbandFMAudio::execute(const buffer_c8_t& buffer) {
	/* Called every 2048/3072000 second -- 1500Hz. */

	auto decimator_out = decimator.execute(buffer);

	const buffer_c16_t work_baseband_buffer {
		(complex16_t*)decimator_out.p,
		sizeof(*decimator_out.p) * decimator_out.count
	};

	/* 96kHz complex<int16_t>[64]
	 * -> FIR filter, <6kHz (0.063fs) pass, gain 1.0
	 * -> 48kHz int16_t[32] */
	auto channel = channel_filter.execute(decimator_out, work_baseband_buffer);

	// TODO: Feed channel_stats post-decimation data?
	feed_channel_stats(channel);
	feed_channel_spectrum(
		channel,
		decimator_out.sampling_rate * channel_filter_taps.pass_frequency_normalized,
		decimator_out.sampling_rate * channel_filter_taps.stop_frequency_normalized
	);

	const buffer_s16_t work_audio_buffer {
		(int16_t*)decimator_out.p,
		sizeof(*decimator_out.p) * decimator_out.count
	};

	/* 48kHz complex<int16_t>[32]
	 * -> FM demodulation
	 * -> 48kHz int16_t[32] */
	auto audio = demod.execute(channel, work_audio_buffer);

	static uint64_t audio_present_history = 0;
	const auto audio_present_now = squelch.execute(audio);
	audio_present_history = (audio_present_history << 1) | (audio_present_now ? 1 : 0);
	const bool audio_present = (audio_present_history != 0);

	if( !audio_present ) {
		// Zero audio buffer.
		for(size_t i=0; i<audio.count; i++) {
			audio.p[i] = 0;
		}
	}

	audio_hpf.execute_in_place(audio);
	fill_audio_buffer(audio);
}
