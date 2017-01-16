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

#include "dsp_types.hpp"
#include "message.hpp"

#include <cstdint>
#include <cstddef>

class AudioStatsCollector {
public:
	template<typename Callback>
	void feed(const buffer_f32_t& src, Callback callback) {
		if( feed(src) ) {
			callback(statistics);
		}
	}

	template<typename Callback>
	void mute(const size_t sample_count, const size_t sampling_rate, Callback callback) {
		if( mute(sample_count, sampling_rate) ) {
			callback(statistics);
		}
	}

private:
	static constexpr float update_interval { 0.1f };
	float squared_sum { 0 };
	float max_squared { 0 };
	size_t count { 0 };

	AudioStatistics statistics { };

	void consume_audio_buffer(const buffer_f32_t& src);

	bool update_stats(const size_t sample_count, const size_t sampling_rate);

	bool feed(const buffer_f32_t& src);
	bool mute(const size_t sample_count, const size_t sampling_rate);
};

#endif/*__AUDIO_STATS_COLLECTOR_H__*/
