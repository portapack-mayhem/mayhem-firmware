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

#include "audio_stats_collector.hpp"

#include "utility.hpp"

void AudioStatsCollector::consume_audio_buffer(const buffer_f32_t& src) {
	auto src_p = src.p;
	const auto src_end = &src.p[src.count];
	while(src_p < src_end) {
		const auto sample = *(src_p++);
		const auto sample_squared = sample * sample;
		squared_sum += sample_squared;
		if( sample_squared > max_squared ) {
			max_squared = sample_squared;
		}
	}
}

bool AudioStatsCollector::update_stats(const size_t sample_count, const size_t sampling_rate) {
	count += sample_count;

	const size_t samples_per_update = sampling_rate * update_interval;

	if( count >= samples_per_update ) {
		statistics.rms_db = mag2_to_dbv_norm(squared_sum / count);
		statistics.max_db = mag2_to_dbv_norm(max_squared);
		statistics.count = count;

		squared_sum = 0;
		max_squared = 0;
		count = 0;

		return true;
	} else {
		return false;
	}
}

bool AudioStatsCollector::feed(const buffer_f32_t& src) {
	consume_audio_buffer(src);

	return update_stats(src.count, src.sampling_rate);
}

bool AudioStatsCollector::mute(const size_t sample_count, const size_t sampling_rate) {
	return update_stats(sample_count, sampling_rate);
}
