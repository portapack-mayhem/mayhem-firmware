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

#ifndef __AUDIO_STATS_COLLECTOR_H__
#define __AUDIO_STATS_COLLECTOR_H__

#include "buffer.hpp"
#include "message.hpp"
#include "utility.hpp"

#include <cstdint>
#include <cstddef>

class AudioStatsCollector {
public:
	template<typename Callback>
	void feed(const buffer_s16_t& src, Callback callback) {
		consume_audio_buffer(src);

		if( update_stats(src.count, src.sampling_rate) ) {
			callback(statistics);
		}
	}

	template<typename Callback>
	void mute(const size_t sample_count, const size_t sampling_rate, Callback callback) {
		if( update_stats(sample_count, sampling_rate) ) {
			callback(statistics);
		}
	}

private:
	static constexpr float update_interval { 0.1f };
	uint64_t squared_sum { 0 };
	uint32_t max_squared { 0 };
	size_t count { 0 };

	AudioStatistics statistics;

	void consume_audio_buffer(const buffer_s16_t& src) {
		auto src_p = src.p;
		const auto src_end = &src.p[src.count];
		while(src_p < src_end) {
			const auto sample = *(src_p++);
			const uint64_t sample_squared = sample * sample;
			squared_sum += sample_squared;
			if( sample_squared > max_squared ) {
				max_squared = sample_squared;
			}
		}
	}

	bool update_stats(const size_t sample_count, const size_t sampling_rate) {
		count += sample_count;

		const size_t samples_per_update = sampling_rate * update_interval;

		if( count >= samples_per_update ) {
			const float squared_sum_f = squared_sum;
			const float max_squared_f = max_squared;
			const float squared_avg_f = squared_sum_f / count;
			statistics.rms_db = complex16_mag_squared_to_dbv_norm(squared_avg_f);
			statistics.max_db = complex16_mag_squared_to_dbv_norm(max_squared_f);
			statistics.count = count;

			squared_sum = 0;
			max_squared = 0;
			count = 0;

			return true;
		} else {
			return false;
		}
	}
};

#endif/*__AUDIO_STATS_COLLECTOR_H__*/
