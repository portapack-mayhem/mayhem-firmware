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

/* 3kHz/6.7kHz @ 96kHz. sum(abs(taps)): 89429 */
constexpr fir_taps_real<64> taps_64_lp_031_070_tfilter {
	.pass_frequency_normalized = 0.031f,
	.stop_frequency_normalized = 0.070f,
	.taps = { {
	    56,     58,     81,    100,    113,    112,     92,     49,
	   -21,   -120,   -244,   -389,   -543,   -692,   -819,   -903,
	  -923,   -861,   -698,   -424,    -34,    469,   1073,   1756,
	  2492,   3243,   3972,   4639,   5204,   5634,   5903,   5995,
	  5903,   5634,   5204,   4639,   3972,   3243,   2492,   1756,
	  1073,    469,    -34,   -424,   -698,   -861,   -923,   -903,
	  -819,   -692,   -543,   -389,   -244,   -120,    -21,     49,
	    92,    112,    113,    100,     81,     58,     56,      0,
	} },
};

/* 4kHz/7.5kHz @ 96kHz. sum(abs(taps)): 96783 */
constexpr fir_taps_real<64> taps_64_lp_042_078_tfilter {
	.pass_frequency_normalized = 0.042f,
	.stop_frequency_normalized = 0.078f,
	.taps = { {
	   -19,     39,     72,    126,    197,    278,    360,    432,
	   478,    485,    438,    327,    152,    -82,   -359,   -651,
	  -922,  -1132,  -1236,  -1192,   -968,   -545,     81,    892,
	  1852,   2906,   3984,   5012,   5910,   6609,   7053,   7205,
	  7053,   6609,   5910,   5012,   3984,   2906,   1852,    892,
	    81,   -545,   -968,  -1192,  -1236,  -1132,   -922,   -651,
	  -359,    -82,    152,    327,    438,    485,    478,    432,
	   360,    278,    197,    126,     72,     39,    -19,      0,
	} },
};

/* 5kHz/8.5kHz @ 96kHz. sum(abs(taps)): 101312 */
constexpr fir_taps_real<64> taps_64_lp_052_089_tfilter {
	.pass_frequency_normalized = 0.052f,
	.stop_frequency_normalized = 0.089f,
	.taps = { {
	   -65,    -88,   -129,   -163,   -178,   -160,   -100,      9,
	   160,    340,    523,    675,    758,    738,    591,    313,
	   -76,   -533,   -987,  -1355,  -1544,  -1472,  -1077,   -335,
	   738,   2078,   3579,   5104,   6502,   7627,   8355,   8608,
	  8355,   7627,   6502,   5104,   3579,   2078,    738,   -335,
	 -1077,  -1472,  -1544,  -1355,   -987,   -533,    -76,    313,
	   591,    738,    758,    675,    523,    340,    160,      9,
	  -100,   -160,   -178,   -163,   -129,    -88,    -65,      0,
	} },
};

/* 6kHz/9.6kHz @ 96kHz. sum(abs(taps)): 105088 */
constexpr fir_taps_real<64> taps_64_lp_063_100_tfilter {
	.pass_frequency_normalized = 0.063f,
	.stop_frequency_normalized = 0.100f,
	.taps = { {
	    43,     21,     -2,    -54,   -138,   -245,   -360,   -453,
	  -493,   -451,   -309,    -73,    227,    535,    776,    876,
	   773,    443,    -86,   -730,  -1357,  -1801,  -1898,  -1515,
	  -585,    869,   2729,   4794,   6805,   8490,   9611,  10004,
	  9611,   8490,   6805,   4794,   2729,    869,   -585,  -1515,
	 -1898,  -1801,  -1357,   -730,    -86,    443,    773,    876,
	   776,    535,    227,    -73,   -309,   -451,   -493,   -453,
	  -360,   -245,   -138,    -54,     -2,     21,     43,      0,
	} },
};

/* 7kHz/10.4kHz @ 96kHz: sum(abs(taps)): 110157 */
constexpr fir_taps_real<64> taps_64_lp_073_108_tfilter {
	.pass_frequency_normalized = 0.073f,
	.stop_frequency_normalized = 0.108f,
	.taps = { {
	    79,    145,    241,    334,    396,    394,    306,    130,
	  -109,   -360,   -550,   -611,   -494,   -197,    229,    677,
	  1011,   1096,    846,    257,   -570,  -1436,  -2078,  -2225,
	 -1670,   -327,   1726,   4245,   6861,   9146,  10704,  11257,
	 10704,   9146,   6861,   4245,   1726,   -327,  -1670,  -2225,
	 -2078,  -1436,   -570,    257,    846,   1096,   1011,    677,
	   229,   -197,   -494,   -611,   -550,   -360,   -109,    130,
	   306,    394,    396,    334,    241,    145,     79,      0,
	} },
};

/* 8kHz/11.5kHz @ 96kHz. sum(abs(taps)): 112092 */
constexpr fir_taps_real<64> taps_64_lp_083_120_tfilter {
	.pass_frequency_normalized = 0.083f,
	.stop_frequency_normalized = 0.120f,
	.taps = { {
	   -63,    -72,    -71,    -21,     89,    248,    417,    537,
	   548,    407,    124,   -237,   -563,   -723,   -621,   -238,
	   337,    919,   1274,   1201,    617,   -382,  -1514,  -2364,
	 -2499,  -1600,    414,   3328,   6651,   9727,  11899,  12682,
	 11899,   9727,   6651,   3328,    414,  -1600,  -2499,  -2364,
	 -1514,   -382,    617,   1201,   1274,    919,    337,   -238,
	  -621,   -723,   -563,   -237,    124,    407,    548,    537,
	   417,    248,     89,    -21,    -71,    -72,    -63,      0,
	} },
};

/* 9kHz/12.4kHz @ 96kHz. sum(abs(taps)): 116249 */
constexpr fir_taps_real<64> taps_64_lp_094_129_tfilter {
	.pass_frequency_normalized = 0.094f,
	.stop_frequency_normalized = 0.129f,
	.taps = { {
	     5,    -93,   -198,   -335,   -449,   -478,   -378,   -144,
	   166,    444,    563,    440,     82,   -395,   -788,   -892,
	  -589,     73,    859,   1421,   1431,    734,   -530,  -1919,
	 -2798,  -2555,   -837,   2274,   6220,  10103,  12941,  13981,
	 12941,  10103,   6220,   2274,   -837,  -2555,  -2798,  -1919,
	  -530,    734,   1431,   1421,    859,     73,   -589,   -892,
	  -788,   -395,     82,    440,    563,    444,    166,   -144,
	  -378,   -478,   -449,   -335,   -198,    -93,      5,      0,
	} },
};

/* 10kHz/13.4kHz @ 96kHz. sum(abs(taps)): 118511 */
constexpr fir_taps_real<64> taps_64_lp_104_140_tfilter {
	.pass_frequency_normalized = 0.104f,
	.stop_frequency_normalized = 0.140f,
	.taps = { {
	    89,    159,    220,    208,     84,   -147,   -412,   -597,
	  -588,   -345,     58,    441,    595,    391,   -128,   -730,
	 -1080,   -914,   -198,    793,   1558,   1594,    678,   -942,
	 -2546,  -3187,  -2084,    992,   5515,  10321,  13985,  15353,
	 13985,  10321,   5515,    992,  -2084,  -3187,  -2546,   -942,
	   678,   1594,   1558,    793,   -198,   -914,  -1080,   -730,
	  -128,    391,    595,    441,     58,   -345,   -588,   -597,
	  -412,   -147,     84,    208,    220,    159,     89,      0,
	} },
};

/* Wideband FM channel filter
 * 103kHz/128kHz @ 768kHz
 */
constexpr fir_taps_real<64> taps_64_lp_130_169_tfilter {
	.pass_frequency_normalized = 0.130f,
	.stop_frequency_normalized = 0.169f,
	.taps = { {
	   100,    127,     62,   -157,   -470,   -707,   -678,   -332,
	   165,    494,    400,    -85,   -610,   -729,   -253,    535,
	  1026,    734,   -263,  -1264,  -1398,   -332,   1316,   2259,
	  1447,   -988,  -3474,  -3769,   -385,   6230,  13607,  18450,
	 18450,  13607,   6230,   -385,  -3769,  -3474,   -988,   1447,
	  2259,   1316,   -332,  -1398,  -1264,   -263,    734,   1026,
	   535,   -253,   -729,   -610,    -85,    400,    494,    165,
	  -332,   -678,   -707,   -470,   -157,     62,    127,    100,
	} },
};

// 41kHz/70kHz @ 192kHz
// http://t-filter.appspot.com
constexpr fir_taps_real<64> taps_64_lp_410_700_tfilter {
	.pass_frequency_normalized = 0.213f,
	.stop_frequency_normalized = 0.364f,
	.taps = { {
	  0,
	  0,
	  0,
	  0,
	  -1,
	  0,
	  3,
	  -3,
	  -7,
	  12,
	  10,
	  -35,
	  -3,
	  79,
	  -37,
	  -138,
	  146,
	  180,
	  -361,
	  -126,
	  688,
	  -149,
	  -1062,
	  800,
	  1308,
	  -1991,
	  -1092,
	  3963,
	  -286,
	  -7710,
	  6211,
	  32368,
	  32368,
	  6211,
	  -7710,
	  -286,
	  3963,
	  -1092,
	  -1991,
	  1308,
	  800,
	  -1062,
	  -149,
	  688,
	  -126,
	  -361,
	  180,
	  146,
	  -138,
	  -37,
	  79,
	  -3,
	  -35,
	  10,
	  12,
	  -7,
	  -3,
	  3,
	  0,
	  -1,
	  0,
	  0,
	  0,
	  0
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
