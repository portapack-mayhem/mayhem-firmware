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

#ifndef __DSP_FIR_TAPS_H__
#define __DSP_FIR_TAPS_H__

#include <cstdint>
#include <array>

#include "complex.hpp"

template<size_t N>
struct fir_taps_real {
	float pass_frequency_normalized;
	float stop_frequency_normalized; 
	std::array<int16_t, N> taps;
};

template<size_t N>
struct fir_taps_complex {
	float pass_frequency_normalized;
	float stop_frequency_normalized; 
	std::array<complex16_t, N> taps;
};

// NBFM 16K0F3E emission type /////////////////////////////////////////////

// IFIR image-reject filter: fs=3072000, pass=8000, stop=344000, decim=8, fout=384000
constexpr fir_taps_real<24> taps_16k0_decim_0 {
	.pass_frequency_normalized =   8000.0f / 3072000.0f,
	.stop_frequency_normalized = 344000.0f / 3072000.0f,
	.taps = { {
		     1,     67,    165,    340,    599,    944,   1361,   1820,
		  2278,   2684,   2988,   3152,   3152,   2988,   2684,   2278,
		  1820,   1361,    944,    599,    340,    165,     67,      1,
	} },
};

// IFIR prototype filter: fs=384000, pass=8000, stop=40000, decim=8, fout=48000
constexpr fir_taps_real<32> taps_16k0_decim_1 {
	.pass_frequency_normalized =  8000.0f / 384000.0f,
	.stop_frequency_normalized = 40000.0f / 384000.0f,
	.taps = { {
		   -26,   -125,   -180,   -275,   -342,   -359,   -286,    -90,
		   250,    733,   1337,   2011,   2688,   3289,   3740,   3982,
		  3982,   3740,   3289,   2688,   2011,   1337,    733,    250,
		   -90,   -286,   -359,   -342,   -275,   -180,   -125,    -26,
	} },
};

// Channel filter: fs=48000, pass=8000, stop=12400, decim=1, fout=48000
constexpr fir_taps_real<32> taps_16k0_channel {
	.pass_frequency_normalized =  8000.0f / 48000.0f,
	.stop_frequency_normalized = 12400.0f / 48000.0f,
	.taps = { {
		   -73,   -285,   -376,     -8,    609,    538,   -584,  -1387,
		  -148,   2173,   1959,  -2146,  -5267,   -297,  12915,  24737,
		 24737,  12915,   -297,  -5267,  -2146,   1959,   2173,   -148,
		 -1387,   -584,    538,    609,     -8,   -376,   -285,    -73,
	} },
};

// NBFM 11K0F3E emission type /////////////////////////////////////////////

// IFIR image-reject filter: fs=3072000, pass=5500, stop=341500, decim=8, fout=384000
constexpr fir_taps_real<24> taps_11k0_decim_0 {
	.pass_frequency_normalized =   5500.0f / 3072000.0f,
	.stop_frequency_normalized = 341500.0f / 3072000.0f,
	.taps = { {
		    38,    102,    220,    406,    668,   1004,   1397,   1822,
		  2238,   2603,   2875,   3020,   3020,   2875,   2603,   2238,
		  1822,   1397,   1004,    668,    406,    220,    102,     38,
	} },
};

// IFIR prototype filter: fs=384000, pass=5500, stop=42500, decim=8, fout=48000
constexpr fir_taps_real<32> taps_11k0_decim_1 {
	.pass_frequency_normalized =  5500.0f / 384000.0f,
	.stop_frequency_normalized = 42500.0f / 384000.0f,
	.taps = { {
		   -42,    -87,   -157,   -234,   -298,   -318,   -255,    -75,
		   246,    713,   1306,   1976,   2656,   3265,   3724,   3971,
		  3971,   3724,   3265,   2656,   1976,   1306,    713,    246,
		   -75,   -255,   -318,   -298,   -234,   -157,    -87,    -42,
	} },
};

// Channel filter: fs=48000, pass=5500, stop=8900, decim=1, fout=48000
constexpr fir_taps_real<32> taps_11k0_channel {
	.pass_frequency_normalized = 5500.0f / 48000.0f,
	.stop_frequency_normalized = 8900.0f / 48000.0f,
	.taps = { {
		   -68,   -345,   -675,   -867,   -582,    247,   1222,   1562,
		   634,  -1379,  -3219,  -3068,    310,   6510,  13331,  17795,
		 17795,  13331,   6510,    310,  -3068,  -3219,  -1379,    634,
		  1562,   1222,    247,   -582,   -867,   -675,   -345,    -68,
	} },
};

/// NBFM 8K50F3E emission type ////////////////////////////////////////////

// IFIR image-reject filter: fs=3072000, pass=4250, stop=340250, decim=8, fout=384000
constexpr fir_taps_real<24> taps_4k25_decim_0 {
	.pass_frequency_normalized =   4250.0f / 3072000.0f,
	.stop_frequency_normalized = 340250.0f / 3072000.0f,
	.taps = { {
		    38,    103,    222,    409,    671,   1006,   1399,   1821,
		  2236,   2599,   2868,   3012,   3012,   2868,   2599,   2236,
		  1821,   1399,   1006,    671,    409,    222,    103,     38,
	} },
};

// IFIR prototype filter: fs=384000, pass=4250, stop=43750, decim=8, fout=48000
constexpr fir_taps_real<32> taps_4k25_decim_1 {
	.pass_frequency_normalized =  4250.0f / 384000.0f,
	.stop_frequency_normalized = 43750.0f / 384000.0f,
	.taps = { {
		   -33,    -74,   -139,   -214,   -280,   -306,   -254,    -87,
		   222,    682,   1274,   1951,   2644,   3268,   3741,   3996,
		  3996,   3741,   3268,   2644,   1951,   1274,    682,    222,
		   -87,   -254,   -306,   -280,   -214,   -139,    -74,    -33,
	} },
};

// Channel filter: fs=48000, pass=4250, stop=7900, decim=1, fout=48000
constexpr fir_taps_real<32> taps_4k25_channel {
	.pass_frequency_normalized = 4250.0f / 48000.0f,
	.stop_frequency_normalized = 7900.0f / 48000.0f,
	.taps = { {
		   -58,    -14,    153,    484,    871,   1063,    770,   -141,
		 -1440,  -2488,  -2435,   -614,   3035,   7771,  12226,  14927,
		 14927,  12226,   7771,   3035,   -614,  -2435,  -2488,  -1440,
		  -141,    770,   1063,    871,    484,    153,    -14,    -58,
	} },
};

// DSB AM 6K00A3E emission type ///////////////////////////////////////////

// IFIR image-reject filter: fs=3072000, pass=3000, stop=339000, decim=8, fout=384000
constexpr fir_taps_real<24> taps_6k0_decim_0 {
	.pass_frequency_normalized =   3000.0f / 3072000.0f,
	.stop_frequency_normalized = 339000.0f / 3072000.0f,
	.taps = { {
		    39,    104,    224,    412,    674,   1008,   1400,   1821,
		  2234,   2594,   2863,   3006,   3006,   2863,   2594,   2234,
		  1821,   1400,   1008,    674,    412,    224,    104,     39,
	} },
};

// IFIR prototype filter: fs=384000, pass=3000, stop=45000, decim=8, fout=48000
constexpr fir_taps_real<32> taps_6k0_decim_1 {
	.pass_frequency_normalized =  3000.0f / 384000.0f,
	.stop_frequency_normalized = 45000.0f / 384000.0f,
	.taps = { {
		   -26,    -63,   -123,   -195,   -263,   -295,   -253,    -99,
		   199,    651,   1242,   1927,   2633,   3273,   3760,   4023,
		  4023,   3760,   3273,   2633,   1927,   1242,    651,    199,
		   -99,   -253,   -295,   -263,   -195,   -123,    -63,    -26,
	} },
};

// Channel filter: fs=48000, pass=3000, stop=6700, decim=1, fout=48000
constexpr fir_taps_real<32> taps_6k0_channel {
	.pass_frequency_normalized = 3000.0f / 48000.0f,
	.stop_frequency_normalized = 6700.0f / 48000.0f,
	.taps = { {
		    95,    178,    247,    208,    -21,   -474,  -1080,  -1640,
		 -1857,  -1411,    -83,   2134,   4978,   7946,  10413,  11815,
		 11815,  10413,   7946,   4978,   2134,    -83,  -1411,  -1857,
		 -1640,  -1080,   -474,    -21,    208,    247,    178,     95,
	} },
};

// USB AM 2K80J3E emission type ///////////////////////////////////////////

// IFIR prototype filter: fs=48000, pass=3000, stop=6700, decim=1, fout=48000
constexpr fir_taps_complex<64> taps_2k8_usb_channel {
	.pass_frequency_normalized = 3000.0f / 12000.0f,
	.stop_frequency_normalized = 3300.0f / 12000.0f,
	.taps = { {
		{  -146,     0 }, {   -41,   -45 }, {    -1,    10 }, {   -95,    69 },
		{  -194,   -41 }, {   -91,  -158 }, {    14,   -43 }, {  -150,    67 },
		{  -299,  -133 }, {  -100,  -307 }, {    50,   -86 }, {  -254,    54 },
		{  -453,  -329 }, {   -62,  -587 }, {   170,  -189 }, {  -334,     0 },
		{  -580,  -645 }, {   104,  -986 }, {   418,  -304 }, {  -412,   -88 },
		{  -680, -1178 }, {   527, -1623 }, {   970,  -432 }, {  -441,  -196 },
		{  -698, -2149 }, {  1617, -2800 }, {  2384,  -507 }, {  -429,  -311 },
		{  -545, -5181 }, {  6925, -7691 }, { 14340,     0 }, { 10601, 11773 },
		{ -1499, 14261 }, { -8373,  6083 }, { -5095, -1083 }, {  -265,  -459 },
		{  -753,  2318 }, { -2954,  1315 }, { -2064,  -919 }, {  -149,  -459 },
		{  -531,   920 }, { -1669,   355 }, { -1100,  -800 }, {   -44,  -419 },
		{  -346,   384 }, {  -992,     0 }, {  -580,  -645 }, {    35,  -332 },
		{  -205,   149 }, {  -577,  -123 }, {  -280,  -485 }, {    80,  -247 },
		{   -91,    40 }, {  -294,  -131 }, {  -101,  -312 }, {    82,  -142 },
		{   -44,     9 }, {  -147,  -107 }, {   -21,  -197 }, {    79,   -88 },
		{    10,     0 }, {   -41,   -45 }, {    15,  -145 }, {     0,     0 },
	} },
};

// WFM 200KF8E emission type //////////////////////////////////////////////

// IFIR image-reject filter: fs=3072000, pass=100000, stop=484000, decim=4, fout=768000
constexpr fir_taps_real<24> taps_200k_wfm_decim_0 = {
	.pass_frequency_normalized = 100000.0f / 3072000.0f,
	.stop_frequency_normalized = 484000.0f / 3072000.0f,
	.taps = { {
		    48,    -18,   -151,   -364,   -557,   -548,   -139,    789,
		  2187,   3800,   5230,   6071,   6071,   5230,   3800,   2187,
		   789,   -139,   -548,   -557,   -364,   -151,    -18,     48,
	} },
};

// IFIR prototype filter: fs=768000, pass=100000, stop=284000, decim=2, fout=384000
constexpr fir_taps_real<16> taps_200k_wfm_decim_1 = {
	.pass_frequency_normalized = 100000.0f / 768000.0f,
	.stop_frequency_normalized = 284000.0f / 768000.0f,
	.taps = { {
		   -67,   -123,    388,    622,  -1342,  -2185,   4599,  14486,
		 14486,   4599,  -2185,  -1342,    622,    388,   -123,    -67,
	} },
};

/* Wideband audio filter */
/* 96kHz int16_t input
 * -> FIR filter, <15kHz (0.156fs) pass, >19kHz (0.198fs) stop
 * -> 48kHz int16_t output, gain of 1.0 (I think).
 * Padded to multiple of four taps for unrolled FIR code.
 * sum(abs(taps)): 125270
 */
constexpr fir_taps_real<64> taps_64_lp_156_198 {
	.pass_frequency_normalized = 0.156f,
	.stop_frequency_normalized = 0.196f,
	.taps = { {
	   -27,    166,    104,    -36,   -174,   -129,    109,    287,
	   148,   -232,   -430,   -130,    427,    597,     49,   -716,
	  -778,    137,   1131,    957,   -493,  -1740,  -1121,   1167,
	  2733,   1252,  -2633,  -4899,  -1336,   8210,  18660,  23254,
	 18660,   8210,  -1336,  -4899,  -2633,   1252,   2733,   1167,
	 -1121,  -1740,   -493,    957,   1131,    137,   -778,   -716,
	    49,    597,    427,   -130,   -430,   -232,    148,    287,
	   109,   -129,   -174,    -36,    104,    166,    -27,      0,
	} },
};

#endif/*__DSP_FIR_TAPS_H__*/
