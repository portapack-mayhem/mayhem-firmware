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

// NBFM 16K0F3E emission type /////////////////////////////////////////////

// IFIR image-reject filter: fs=3072000, pass=8000, stop=344000, decim=8, fout=384000
static constexpr std::array<int16_t, 24> taps_16k0_decim_0 { {
	     1,     67,    165,    340,    599,    944,   1361,   1820,
	  2278,   2684,   2988,   3152,   3152,   2988,   2684,   2278,
	  1820,   1361,    944,    599,    340,    165,     67,      1,
} };

// IFIR prototype filter: fs=384000, pass=8000, stop=40000, decim=8, fout=48000
static constexpr std::array<int16_t, 32> taps_16k0_decim_1 { {
	   -26,   -125,   -180,   -275,   -342,   -359,   -286,    -90,
	   250,    733,   1337,   2011,   2688,   3289,   3740,   3982,
	  3982,   3740,   3289,   2688,   2011,   1337,    733,    250,
	   -90,   -286,   -359,   -342,   -275,   -180,   -125,    -26,
} };

// Channel filter: fs=48000, pass=8000, stop=12400, decim=1, fout=48000
static constexpr std::array<int16_t, 32> taps_16k0_channel { {
	   -73,   -285,   -376,     -8,    609,    538,   -584,  -1387,
	  -148,   2173,   1959,  -2146,  -5267,   -297,  12915,  24737,
	 24737,  12915,   -297,  -5267,  -2146,   1959,   2173,   -148,
	 -1387,   -584,    538,    609,     -8,   -376,   -285,    -73,
} };

// NBFM 11K0F3E emission type /////////////////////////////////////////////

// IFIR image-reject filter: fs=3072000, pass=5500, stop=341500, decim=8, fout=384000
static constexpr std::array<int16_t, 24> taps_11k0_decim_0 { {
	    38,    102,    220,    406,    668,   1004,   1397,   1822,
	  2238,   2603,   2875,   3020,   3020,   2875,   2603,   2238,
	  1822,   1397,   1004,    668,    406,    220,    102,     38,
} };

// IFIR prototype filter: fs=384000, pass=5500, stop=42500, decim=8, fout=48000
static constexpr std::array<int16_t, 32> taps_11k0_decim_1 { {
	   -42,    -87,   -157,   -234,   -298,   -318,   -255,    -75,
	   246,    713,   1306,   1976,   2656,   3265,   3724,   3971,
	  3971,   3724,   3265,   2656,   1976,   1306,    713,    246,
	   -75,   -255,   -318,   -298,   -234,   -157,    -87,    -42,
} };

// Channel filter: fs=48000, pass=5500, stop=8900, decim=1, fout=48000
static constexpr std::array<int16_t, 32> taps_11k0_channel { {
	   -68,   -345,   -675,   -867,   -582,    247,   1222,   1562,
	   634,  -1379,  -3219,  -3068,    310,   6510,  13331,  17795,
	 17795,  13331,   6510,    310,  -3068,  -3219,  -1379,    634,
	  1562,   1222,    247,   -582,   -867,   -675,   -345,    -68,
} };

/// NBFM 8K50F3E emission type ////////////////////////////////////////////

// IFIR image-reject filter: fs=3072000, pass=4250, stop=340250, decim=8, fout=384000
static constexpr std::array<int16_t, 24> taps_4k25_decim_0 { {
	    38,    103,    222,    409,    671,   1006,   1399,   1821,
	  2236,   2599,   2868,   3012,   3012,   2868,   2599,   2236,
	  1821,   1399,   1006,    671,    409,    222,    103,     38,
} };

// IFIR prototype filter: fs=384000, pass=4250, stop=43750, decim=8, fout=48000
static constexpr std::array<int16_t, 32> taps_4k25_decim_1 { {
	   -33,    -74,   -139,   -214,   -280,   -306,   -254,    -87,
	   222,    682,   1274,   1951,   2644,   3268,   3741,   3996,
	  3996,   3741,   3268,   2644,   1951,   1274,    682,    222,
	   -87,   -254,   -306,   -280,   -214,   -139,    -74,    -33,
} };

// Channel filter: fs=48000, pass=4250, stop=7900, decim=1, fout=48000
static constexpr std::array<int16_t, 32> taps_4k25_channel { {
	   -58,    -14,    153,    484,    871,   1063,    770,   -141,
	 -1440,  -2488,  -2435,   -614,   3035,   7771,  12226,  14927,
	 14927,  12226,   7771,   3035,   -614,  -2435,  -2488,  -1440,
	  -141,    770,   1063,    871,    484,    153,    -14,    -58,
} };

NarrowbandFMAudio::NarrowbandFMAudio() {
	set_mode(Mode::Deviation2K5Narrow);
}

void NarrowbandFMAudio::set_mode(const Mode mode) {
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

	constexpr size_t demod_input_fs = channel_filter_output_fs;

	switch(mode) {
	default:
	case Mode::Deviation5K:
		decim_0.configure(taps_16k0_decim_0, 33554432);
		decim_1.configure(taps_16k0_decim_1, 131072);
		channel_filter.configure(taps_16k0_channel, channel_filter_decimation_factor);
		demod.configure(demod_input_fs, 5000);
		channel_filter_pass_f =  8000;
		channel_filter_stop_f = 12400;
		break;

	case Mode::Deviation2K5Wide:
		decim_0.configure(taps_11k0_decim_0, 33554432);
		decim_1.configure(taps_11k0_decim_1, 131072);
		channel_filter.configure(taps_11k0_channel, channel_filter_decimation_factor);
		demod.configure(demod_input_fs, 2500);
		channel_filter_pass_f = 5500;
		channel_filter_stop_f = 8900;
		break;

	case Mode::Deviation2K5Narrow:
		decim_0.configure(taps_4k25_decim_0, 33554432);
		decim_1.configure(taps_4k25_decim_1, 131072);
		channel_filter.configure(taps_4k25_channel, channel_filter_decimation_factor);
		demod.configure(demod_input_fs, 2500);
		channel_filter_pass_f = 4250;
		channel_filter_stop_f = 7900;
		break;
	}

	channel_spectrum.set_decimation_factor(std::floor((channel_filter_output_fs / 2) / ((channel_filter_pass_f + channel_filter_stop_f) / 2)));
}

void NarrowbandFMAudio::execute(const buffer_c8_t& buffer) {
	const buffer_c16_t dst_buffer {
		dst.data(),
		dst.size()
	};

	const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
	const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
	const auto channel_out = channel_filter.execute(decim_1_out, dst_buffer);

	feed_channel_stats(channel_out);
	channel_spectrum.feed(channel_out, channel_filter_pass_f, channel_filter_stop_f);

	const buffer_s16_t work_audio_buffer {
		(int16_t*)dst.data(),
		sizeof(*dst.data()) * dst.size()
	};

	auto audio = demod.execute(channel_out, work_audio_buffer);

	static uint64_t audio_present_history = 0;
	const auto audio_present_now = squelch.execute(audio);
	audio_present_history = (audio_present_history << 1) | (audio_present_now ? 1 : 0);
	const bool audio_present = (audio_present_history != 0);

	audio_hpf.execute_in_place(audio);
	audio_deemph.execute_in_place(audio);

	if( !audio_present ) {
		// Zero audio buffer.
		for(size_t i=0; i<audio.count; i++) {
			audio.p[i] = 0;
		}
	}

	fill_audio_buffer(audio);
}
