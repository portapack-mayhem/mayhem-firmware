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

#include "proc_am_audio.hpp"

// DSB AM 6K00A3E emission type ///////////////////////////////////////////

// IFIR image-reject filter: fs=3072000, pass=3000, stop=339000, decim=8, fout=384000
static constexpr std::array<int16_t, 24> taps_6k0_decim_0 { {
	    39,    104,    224,    412,    674,   1008,   1400,   1821,
	  2234,   2594,   2863,   3006,   3006,   2863,   2594,   2234,
	  1821,   1400,   1008,    674,    412,    224,    104,     39,
} };

// IFIR prototype filter: fs=384000, pass=3000, stop=45000, decim=8, fout=48000
static constexpr std::array<int16_t, 32> taps_6k0_decim_1 { {
	   -26,    -63,   -123,   -195,   -263,   -295,   -253,    -99,
	   199,    651,   1242,   1927,   2633,   3273,   3760,   4023,
	  4023,   3760,   3273,   2633,   1927,   1242,    651,    199,
	   -99,   -253,   -295,   -263,   -195,   -123,    -63,    -26,
} };

// Channel filter: fs=48000, pass=3000, stop=6700, decim=1, fout=48000
static constexpr std::array<int16_t, 32> taps_6k0_channel { {
	    95,    178,    247,    208,    -21,   -474,  -1080,  -1640,
	 -1857,  -1411,    -83,   2134,   4978,   7946,  10413,  11815,
	 11815,  10413,   7946,   4978,   2134,    -83,  -1411,  -1857,
	 -1640,  -1080,   -474,    -21,    208,    247,    178,     95,
} };

NarrowbandAMAudio::NarrowbandAMAudio() {
	constexpr size_t baseband_fs = 3072000;

	constexpr size_t decim_0_input_fs = baseband_fs;
	constexpr size_t decim_0_decimation_factor = 8;
	constexpr size_t decim_0_output_fs = decim_0_input_fs / decim_0_decimation_factor;

	constexpr size_t decim_1_input_fs = decim_0_output_fs;
	constexpr size_t decim_1_decimation_factor = 8;
	constexpr size_t decim_1_output_fs = decim_1_input_fs / decim_1_decimation_factor;

	constexpr size_t channel_filter_input_fs = decim_1_output_fs;
	constexpr size_t channel_filter_decimation_factor = 1;
	constexpr size_t channel_filter_output_fs = channel_filter_input_fs / channel_filter_decimation_factor;

	decim_0.configure(taps_6k0_decim_0, 33554432);
	decim_1.configure(taps_6k0_decim_1, 131072);
	channel_filter.configure(taps_6k0_channel, 1);
	channel_filter_pass_f = 3000;
	channel_filter_stop_f = 6700;

	channel_spectrum.set_decimation_factor(std::floor((channel_filter_output_fs / 2) / ((channel_filter_pass_f + channel_filter_stop_f) / 2)));
}

void NarrowbandAMAudio::execute(const buffer_c8_t& buffer) {
	const buffer_c16_t dst_buffer {
		dst.data(),
		dst.size()
	};

	const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
	const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
	const auto channel_out = channel_filter.execute(decim_1_out, dst_buffer);

	// TODO: Feed channel_stats post-decimation data?
	feed_channel_stats(channel_out);
	channel_spectrum.feed(channel_out, channel_filter_pass_f, channel_filter_stop_f);

	const buffer_s16_t work_audio_buffer {
		(int16_t*)dst.data(),
		sizeof(*dst.data()) * dst.size()
	};

	auto audio = demod.execute(channel_out, work_audio_buffer);

	audio_hpf.execute_in_place(audio);

	fill_audio_buffer(audio);
}
