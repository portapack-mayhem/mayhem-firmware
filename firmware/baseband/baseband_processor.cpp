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

void BasebandProcessor::feed_channel_stats(const buffer_c16_t& channel) {
	channel_stats.feed(
		channel,
		[](const ChannelStatistics& statistics) {
			const ChannelStatisticsMessage channel_stats_message { statistics };
			shared_memory.application_queue.push(channel_stats_message);
		}
	);
}

void BasebandProcessor::fill_audio_buffer(const buffer_s16_t& audio) {
	auto audio_buffer = audio::dma::tx_empty_buffer();;
	for(size_t i=0; i<audio_buffer.count; i++) {
		audio_buffer.p[i].left = audio_buffer.p[i].right = audio.p[i];
	}
	i2s::i2s0::tx_unmute();

	feed_audio_stats(audio);
}

void BasebandProcessor::feed_audio_stats(const buffer_s16_t& audio) {
	audio_stats.feed(
		audio,
		[](const AudioStatistics& statistics) {
			const AudioStatisticsMessage audio_stats_message { statistics };
			shared_memory.application_queue.push(audio_stats_message);
		}
	);
}
