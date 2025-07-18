/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

template <size_t N>
struct fir_taps_real {
    float low_frequency_normalized;
    float high_frequency_normalized;
    float transition_normalized;
    std::array<int16_t, N> taps;
};

template <size_t N>
struct fir_taps_complex {
    float low_frequency_normalized;
    float high_frequency_normalized;
    float transition_normalized;
    std::array<complex16_t, N> taps;
};

// NBFM 16K0F3E emission type /////////////////////////////////////////////

// IFIR image-reject filter: fs=3072000, pass=8000, stop=344000, decim=8, fout=384000
constexpr fir_taps_real<24> taps_16k0_decim_0{
    .low_frequency_normalized = -8000.0f / 3072000.0f,
    .high_frequency_normalized = 8000.0f / 3072000.0f,
    .transition_normalized = 336000.0f / 3072000.0f,
    .taps = {{
        1,
        67,
        165,
        340,
        599,
        944,
        1361,
        1820,
        2278,
        2684,
        2988,
        3152,
        3152,
        2988,
        2684,
        2278,
        1820,
        1361,
        944,
        599,
        340,
        165,
        67,
        1,
    }},
};

// IFIR prototype filter: fs=384000, pass=8000, stop=40000, decim=8, fout=48000
constexpr fir_taps_real<32> taps_16k0_decim_1{
    .low_frequency_normalized = -8000.0f / 384000.0f,
    .high_frequency_normalized = 8000.0f / 384000.0f,
    .transition_normalized = 32000.0f / 384000.0f,
    .taps = {{
        -26,
        -125,
        -180,
        -275,
        -342,
        -359,
        -286,
        -90,
        250,
        733,
        1337,
        2011,
        2688,
        3289,
        3740,
        3982,
        3982,
        3740,
        3289,
        2688,
        2011,
        1337,
        733,
        250,
        -90,
        -286,
        -359,
        -342,
        -275,
        -180,
        -125,
        -26,
    }},
};

// Channel filter: fs=48000, pass=8000, stop=12400, decim=1, fout=48000
constexpr fir_taps_real<32> taps_16k0_channel{
    .low_frequency_normalized = -8000.0f / 48000.0f,
    .high_frequency_normalized = 8000.0f / 48000.0f,
    .transition_normalized = 4400.0f / 48000.0f,
    .taps = {{
        -73,
        -285,
        -376,
        -8,
        609,
        538,
        -584,
        -1387,
        -148,
        2173,
        1959,
        -2146,
        -5267,
        -297,
        12915,
        24737,
        24737,
        12915,
        -297,
        -5267,
        -2146,
        1959,
        2173,
        -148,
        -1387,
        -584,
        538,
        609,
        -8,
        -376,
        -285,
        -73,
    }},
};

// NBFM 11K0F3E emission type /////////////////////////////////////////////

// IFIR image-reject filter: fs=3072000, pass=5500, stop=341500, decim=8, fout=384000
constexpr fir_taps_real<24> taps_11k0_decim_0{
    .low_frequency_normalized = -5500.0f / 3072000.0f,
    .high_frequency_normalized = 5500.0f / 3072000.0f,
    .transition_normalized = 336000.0f / 3072000.0f,
    .taps = {{
        38,
        102,
        220,
        406,
        668,
        1004,
        1397,
        1822,
        2238,
        2603,
        2875,
        3020,
        3020,
        2875,
        2603,
        2238,
        1822,
        1397,
        1004,
        668,
        406,
        220,
        102,
        38,
    }},
};

// IFIR prototype filter: fs=384000, pass=5500, stop=42500, decim=8, fout=48000
constexpr fir_taps_real<32> taps_11k0_decim_1{
    .low_frequency_normalized = -5500.0f / 384000.0f,
    .high_frequency_normalized = 5500.0f / 384000.0f,
    .transition_normalized = 37000.0f / 384000.0f,
    .taps = {{
        -42,
        -87,
        -157,
        -234,
        -298,
        -318,
        -255,
        -75,
        246,
        713,
        1306,
        1976,
        2656,
        3265,
        3724,
        3971,
        3971,
        3724,
        3265,
        2656,
        1976,
        1306,
        713,
        246,
        -75,
        -255,
        -318,
        -298,
        -234,
        -157,
        -87,
        -42,
    }},
};

// Channel filter: fs=48000, pass=5500, stop=8900, decim=1, fout=48000
constexpr fir_taps_real<32> taps_11k0_channel{
    .low_frequency_normalized = -5500.0f / 48000.0f,
    .high_frequency_normalized = 5500.0f / 48000.0f,
    .transition_normalized = 3400.0f / 48000.0f,
    .taps = {{
        -68,
        -345,
        -675,
        -867,
        -582,
        247,
        1222,
        1562,
        634,
        -1379,
        -3219,
        -3068,
        310,
        6510,
        13331,
        17795,
        17795,
        13331,
        6510,
        310,
        -3068,
        -3219,
        -1379,
        634,
        1562,
        1222,
        247,
        -582,
        -867,
        -675,
        -345,
        -68,
    }},
};

// NBFM 8K50F3E emission type /////////////////////////////////////////////

// IFIR image-reject filter: fs=3072000, pass=4250, stop=340250, decim=8, fout=384000
constexpr fir_taps_real<24> taps_4k25_decim_0{
    .low_frequency_normalized = -4250.0f / 3072000.0f,
    .high_frequency_normalized = 4250.0f / 3072000.0f,
    .transition_normalized = 33600.0f / 3072000.0f,
    .taps = {{
        38,
        103,
        222,
        409,
        671,
        1006,
        1399,
        1821,
        2236,
        2599,
        2868,
        3012,
        3012,
        2868,
        2599,
        2236,
        1821,
        1399,
        1006,
        671,
        409,
        222,
        103,
        38,
    }},
};

// IFIR prototype filter: fs=384000, pass=4250, stop=43750, decim=8, fout=48000
constexpr fir_taps_real<32> taps_4k25_decim_1{
    .low_frequency_normalized = -4250.0f / 384000.0f,
    .high_frequency_normalized = 4250.0f / 384000.0f,
    .transition_normalized = 39500.0f / 384000.0f,
    .taps = {{
        -33,
        -74,
        -139,
        -214,
        -280,
        -306,
        -254,
        -87,
        222,
        682,
        1274,
        1951,
        2644,
        3268,
        3741,
        3996,
        3996,
        3741,
        3268,
        2644,
        1951,
        1274,
        682,
        222,
        -87,
        -254,
        -306,
        -280,
        -214,
        -139,
        -74,
        -33,
    }},
};

// Channel filter: fs=48000, pass=4250, stop=7900, decim=1, fout=48000
constexpr fir_taps_real<32> taps_4k25_channel{
    .low_frequency_normalized = -4250.0f / 48000.0f,
    .high_frequency_normalized = 4250.0f / 48000.0f,
    .transition_normalized = 3650.0f / 48000.0f,
    .taps = {{
        -58,
        -14,
        153,
        484,
        871,
        1063,
        770,
        -141,
        -1440,
        -2488,
        -2435,
        -614,
        3035,
        7771,
        12226,
        14927,
        14927,
        12226,
        7771,
        3035,
        -614,
        -2435,
        -2488,
        -1440,
        -141,
        770,
        1063,
        871,
        484,
        153,
        -14,
        -58,
    }},
};

/* CTCSS audio filter */
/* 12kHz int16_t input
 * -> FIR filter, <300Hz pass, >300Hz stop, gain of 1
 * -> 6kHz int16_t output, gain of 1.0 (I think).
 * Padded to multiple of four taps for unrolled FIR code.
 * sum(abs(taps)): 125270
 */
/*constexpr fir_taps_real<64> taps_64_lp_025_025 {
        .taps = { {
                     0,      0,     -3,     -7,    -13,    -20,    -27,    -32,
                   -34,    -33,    -25,    -10,     13,     47,     94,    152,
                   223,    307,    402,    508,    622,    742,    866,    991,
                  1113,   1229,   1336,   1430,   1510,   1571,   1614,   1635,
                  1635,   1614,   1571,   1510,   1430,   1336,   1229,   1113,
                   991,    866,    742,    622,    508,    402,    307,    223,
                   152,     94,     47,     13,    -10,    -25,    -33,    -34,
                   -32,    -27,    -20,    -13,     -7,     -3,      0,      0
        } },
};*/

/* CTCSS audio filter */
/* 24kHz int16_t input
 * -> FIR filter, <300Hz pass, >300Hz stop, gain of 1
 * -> 12kHz int16_t output, gain of 1.0 (I think).
 * Padded to multiple of four taps for unrolled FIR code.
 * sum(abs(taps)): 125270
 */
constexpr fir_taps_real<64> taps_64_lp_025_025{
    .low_frequency_normalized = 0,
    .high_frequency_normalized = 0,
    .transition_normalized = 0,
    .taps = {{0, 0, 2, 6, 12, 20, 32, 46,
              64, 85, 110, 138, 169, 204, 241, 281,
              323, 367, 412, 457, 502, 547, 590, 631,
              669, 704, 735, 762, 784, 801, 812, 818,
              818, 812, 801, 784, 762, 735, 704, 669,
              631, 590, 547, 502, 457, 412, 367, 323,
              281, 241, 204, 169, 138, 110, 85, 64,
              46, 32, 20, 12, 6, 2, 0, 0}},
};

// DSB AM 6K00A3E emission type ///////////////////////////////////////////

// IFIR image-reject filter: fs=3072000, pass=3000, stop=339000, decim=8, fout=384000
constexpr fir_taps_real<24> taps_6k0_decim_0{
    .low_frequency_normalized = -3000.0f / 3072000.0f,
    .high_frequency_normalized = 3000.0f / 3072000.0f,
    .transition_normalized = 336000.0f / 3072000.0f,
    .taps = {{
        39,
        104,
        224,
        412,
        674,
        1008,
        1400,
        1821,
        2234,
        2594,
        2863,
        3006,
        3006,
        2863,
        2594,
        2234,
        1821,
        1400,
        1008,
        674,
        412,
        224,
        104,
        39,
    }},
};

// IFIR prototype filter: fs=384000, pass=3000, stop=45000, decim=8, fout=48000
constexpr fir_taps_real<32> taps_6k0_decim_1{
    .low_frequency_normalized = -3000.0f / 384000.0f,
    .high_frequency_normalized = 3000.0f / 384000.0f,
    .transition_normalized = 43000.0f / 384000.0f,
    .taps = {{
        -26,
        -63,
        -123,
        -195,
        -263,
        -295,
        -253,
        -99,
        199,
        651,
        1242,
        1927,
        2633,
        3273,
        3760,
        4023,
        4023,
        3760,
        3273,
        2633,
        1927,
        1242,
        651,
        199,
        -99,
        -253,
        -295,
        -263,
        -195,
        -123,
        -63,
        -26,
    }},
};

// IFIR prototype filter: fs=384000, pass=3000, stop=33000, decim=8, fout=48000
// Narrower taps_6k0_decim_1IFIR version to avoid LCD waterfall  aliasing in AMFM Wefax in ZOOM X 2 (means spectrum decimation factor x2)
// It has BW -3dB's of +-9Khz, Stop band from 33khz onwards -60 dB's , then we can use in all AM modes (DSB, SSB,CW )
constexpr fir_taps_real<32> taps_6k0_narrow_decim_1{
    .low_frequency_normalized = -3000.0f / 384000.0f,
    .high_frequency_normalized = 3000.0f / 384000.0f,
    .transition_normalized = 30000.0f / 384000.0f,
    .taps = {{

        58,
        80,
        138,
        219,
        326,
        461,
        622,
        807,
        1011,
        1224,
        1438,
        1640,
        1820,
        1966,
        2069,
        2122,
        2122,
        2069,
        1966,
        1820,
        1640,
        1438,
        1224,
        1011,
        807,
        622,
        461,
        326,
        219,
        138,
        80,
        58,

    }},
};

// IFIR prototype filter: fs=48000, pass=3000, stop=6700, decim=4, fout=12000
constexpr fir_taps_real<32> taps_6k0_decim_2{
    .low_frequency_normalized = -3000.0f / 48000.0f,
    .high_frequency_normalized = 3000.0f / 48000.0f,
    .transition_normalized = 3700.0f / 48000.0f,
    .taps = {{
        95,
        178,
        247,
        208,
        -21,
        -474,
        -1080,
        -1640,
        -1857,
        -1411,
        -83,
        2134,
        4978,
        7946,
        10413,
        11815,
        11815,
        10413,
        7946,
        4978,
        2134,
        -83,
        -1411,
        -1857,
        -1640,
        -1080,
        -474,
        -21,
        208,
        247,
        178,
        95,
    }},
};

// IFIR prototype filter fs=48000 ; pass=4500 (cutoff -3dBs) , stop=8000 (<-60dBs), decim=4, fout=12000
// For Europe AM commercial  broadcasting stations in LF/MF/HF, Emissions Designator 9K00A3E Bandwidth: 9.00 kHz (derivated from taps_6k0_decim_2 )
// Pre-decimate LPF FIR filter design Created with SciPy Python with the "window method", num_taps = 32, cutoff = 5150. sample_rate = 48000 # Hz,
// Created with h = signal.firwin(num_taps, cutoff, nyq=sample_rate/2, window=('chebwin',50)) , achieving good  STOP band plot < -60 dB's with some ripple.
// post-scaled h taps to avoid decimals , targeting <= similar int values as previous taps_6k0_dsb_channel peak < 32.767 (2 exp 15) and similar H(f)gain
constexpr fir_taps_real<32> taps_9k0_decim_2{
    .low_frequency_normalized = -4500.0f / 48000.0f,  // Negative -cutoff freq -3dB (real achieved data ,in the plot and measurements)
    .high_frequency_normalized = 4500.0f / 48000.0f,  // Positive +cutoff freq -3dB (idem)
    .transition_normalized = 3500.0f / 48000.0f,      // 3500 Hz = (8000 Hz - 4500 Hz) (both from plot H(f) curve plot)
    .taps = {{-53, -30, 47, 198, 355, 372, 89, -535,
              -1307, -1771, -1353, 370, 3384, 7109, 10535, 12591,
              12591, 10535, 7109, 3384, 370, -1353, -1771, -1307,
              -535, 89, 372, 355, 198, 47, -30, -53}},
};

// Channel filter: fs=12000, pass=3000, stop=3300, decim=1, fout=12000
/* NOTE: Slightly less than 1.0 gain (normalized to 65536) due to max(taps) being
 * slightly larger than 32767 (33312).
 */
constexpr fir_taps_complex<64> taps_6k0_dsb_channel{
    .low_frequency_normalized = -3000.0f / 12000.0f,
    .high_frequency_normalized = 3000.0f / 12000.0f,
    .transition_normalized = 300.0f / 12000.0f,
    .taps = {{
        {-69, 0},
        {-140, 0},
        {119, 0},
        {89, 0},
        {-132, 0},
        {-134, 0},
        {197, 0},
        {167, 0},
        {-273, 0},
        {-206, 0},
        {372, 0},
        {247, 0},
        {-497, 0},
        {-289, 0},
        {654, 0},
        {331, 0},
        {-854, 0},
        {-372, 0},
        {1112, 0},
        {411, 0},
        {-1455, 0},
        {-446, 0},
        {1933, 0},
        {476, 0},
        {-2654, 0},
        {-501, 0},
        {3902, 0},
        {520, 0},
        {-6717, 0},
        {-531, 0},
        {20478, 0},
        {32767, 0},
        {20478, 0},
        {-531, 0},
        {-6717, 0},
        {520, 0},
        {3902, 0},
        {-501, 0},
        {-2654, 0},
        {476, 0},
        {1933, 0},
        {-446, 0},
        {-1455, 0},
        {411, 0},
        {1112, 0},
        {-372, 0},
        {-854, 0},
        {331, 0},
        {654, 0},
        {-289, 0},
        {-497, 0},
        {247, 0},
        {372, 0},
        {-206, 0},
        {-273, 0},
        {167, 0},
        {197, 0},
        {-134, 0},
        {-132, 0},
        {89, 0},
        {119, 0},
        {-140, 0},
        {-69, 0},
        {0, 0},
    }},
};

// Channel filter: fs=12000, pass=4500 (cutoff -3dBs), stop=4940 (<-60dBs), decim=1, fout=12000   (*1) real frec pass / stop , based on plotted  H(f) curve)
// For Europe AM commercial broadcasting stations in LF/MF/HF, Emissions Designator 9K00A3E Bandwidth: 9.00 kHz (derivative from  taps_6k0_dsb_channel)
// FIR filter design created with SciPy Python using "window method"; selected design parameters: num_taps = 64, cutoff = 4575. sample_rate = 12000 # Hz,
// Created with : h = signal.firwin(num_taps, cutoff, nyq=sample_rate/2, window=('chebwin',50)) , achieving real plot curve (*1) with peak stop band ripple -60dBs.
// post-scaled h taps to avoid decimals , targeting <= similar int values as previous taps_6k0_dsb_channel peak < 32.767 (2 exp 15), (29625)  and similar H(f)gain
constexpr fir_taps_complex<64> taps_9k0_dsb_channel{
    .low_frequency_normalized = -4500.0f / 12000.0f,  // Negative -cutoff freq -3dB (in the H(f) curve plot)
    .high_frequency_normalized = 4500.0f / 12000.0f,  // Positive +cutoff freq -3dB (in the H(f) curve plot)
    .transition_normalized = 440.0f / 12000.0f,       // 440Hz = (4940 Hz -4500 Hz)  cut-3dB's (both data comes from H(f) curve plot and confirmed by  measurements )
    .taps = {{
        {2, 0},
        {-18, 0},
        {34, 0},
        {-33, 0},
        {6, 0},
        {44, 0},
        {-91, 0},
        {96, 0},
        {-35, 0},
        {-80, 0},
        {193, 0},
        {-223, 0},
        {116, 0},
        {112, 0},
        {-353, 0},
        {452, 0},
        {-293, 0},
        {-111, 0},
        {584, 0},
        {-844, 0},
        {653, 0},
        {22, 0},
        {-921, 0},
        {1554, 0},
        {-1422, 0},
        {301, 0},
        {1533, 0},
        {-3282, 0},
        {3804, 0},
        {-1819, 0},
        {-4605, 0},
        {29625, 0},
        {29625, 0},
        {-4605, 0},
        {-1819, 0},
        {3804, 0},
        {-3282, 0},
        {1533, 0},
        {301, 0},
        {-1422, 0},
        {1554, 0},
        {-921, 0},
        {22, 0},
        {653, 0},
        {-844, 0},
        {584, 0},
        {-111, 0},
        {-293, 0},
        {452, 0},
        {-353, 0},
        {112, 0},
        {116, 0},
        {-223, 0},
        {193, 0},
        {-80, 0},
        {-35, 0},
        {96, 0},
        {-91, 0},
        {44, 0},
        {6, 0},
        {-33, 0},
        {34, 0},
        {-18, 0},
        {2, 0},
    }},
};

// USB AM 2K80J3E emission type ///////////////////////////////////////////

// IFIR prototype filter: fs=12000, pass=3000, stop=3300, decim=1, fout=12000
constexpr fir_taps_complex<64> taps_2k8_usb_channel{
    .low_frequency_normalized = 0,
    .high_frequency_normalized = 3000.0f / 12000.0f,
    .transition_normalized = 300.0f / 12000.0f,
    .taps = {{
        {-146, 0},
        {-41, -45},
        {-1, 10},
        {-95, 69},
        {-194, -41},
        {-91, -158},
        {14, -43},
        {-150, 67},
        {-299, -133},
        {-100, -307},
        {50, -86},
        {-254, 54},
        {-453, -329},
        {-62, -587},
        {170, -189},
        {-334, 0},
        {-580, -645},
        {104, -986},
        {418, -304},
        {-412, -88},
        {-680, -1178},
        {527, -1623},
        {970, -432},
        {-441, -196},
        {-698, -2149},
        {1617, -2800},
        {2384, -507},
        {-429, -311},
        {-545, -5181},
        {6925, -7691},
        {14340, 0},
        {10601, 11773},
        {-1499, 14261},
        {-8373, 6083},
        {-5095, -1083},
        {-265, -459},
        {-753, 2318},
        {-2954, 1315},
        {-2064, -919},
        {-149, -459},
        {-531, 920},
        {-1669, 355},
        {-1100, -800},
        {-44, -419},
        {-346, 384},
        {-992, 0},
        {-580, -645},
        {35, -332},
        {-205, 149},
        {-577, -123},
        {-280, -485},
        {80, -247},
        {-91, 40},
        {-294, -131},
        {-101, -312},
        {82, -142},
        {-44, 9},
        {-147, -107},
        {-21, -197},
        {79, -88},
        {10, 0},
        {-41, -45},
        {15, -145},
        {0, 0},
    }},
};

// LSB AM 2K80J3E emission type ///////////////////////////////////////////

// IFIR prototype filter: fs=12000, pass=3000, stop=3300, decim=1, fout=12000
constexpr fir_taps_complex<64> taps_2k8_lsb_channel{
    .low_frequency_normalized = -3000.0f / 12000.0f,
    .high_frequency_normalized = 0,
    .transition_normalized = 300.0f / 12000.0f,
    .taps = {{
        {-146, 0},
        {-41, 45},
        {-1, -10},
        {-95, -69},
        {-194, 41},
        {-91, 158},
        {14, 43},
        {-150, -67},
        {-299, 133},
        {-100, 307},
        {50, 86},
        {-254, -54},
        {-453, 329},
        {-62, 587},
        {170, 189},
        {-334, 0},
        {-580, 645},
        {104, 986},
        {418, 304},
        {-412, 88},
        {-680, 1178},
        {527, 1623},
        {970, 432},
        {-441, 196},
        {-698, 2149},
        {1617, 2800},
        {2384, 507},
        {-429, 311},
        {-545, 5181},
        {6925, 7691},
        {14340, 0},
        {10601, -11773},
        {-1499, -14261},
        {-8373, -6083},
        {-5095, 1083},
        {-265, 459},
        {-753, -2318},
        {-2954, -1315},
        {-2064, 919},
        {-149, 459},
        {-531, -920},
        {-1669, -355},
        {-1100, 800},
        {-44, 419},
        {-346, -384},
        {-992, 0},
        {-580, 645},
        {35, 332},
        {-205, -149},
        {-577, 123},
        {-280, 485},
        {80, 247},
        {-91, -40},
        {-294, 131},
        {-101, 312},
        {82, 142},
        {-44, -9},
        {-147, 107},
        {-21, 197},
        {79, 88},
        {10, 0},
        {-41, 45},
        {15, 145},
        {0, 0},
    }},
};

// USB AM 700Hz filter: fs=12000, start=600, end=800, width=200, stop=40db, decim=1, fout=12000

constexpr fir_taps_complex<64> taps_0k7_usb_channel{
    .low_frequency_normalized = 600.0f / 12000.0f,
    .high_frequency_normalized = 800.0f / 12000.0f,
    .transition_normalized = 200.0f / 12000.0f,
    .taps = {{
        {531, 0},
        {192, 73},
        {181, 163},
        {129, 254},
        {34, 328},
        {-97, 364},
        {-251, 345},
        {-403, 261},
        {-524, 111},
        {-585, -92},
        {-564, -326},
        {-448, -554},
        {-239, -737},
        {43, -836},
        {366, -822},
        {681, -681},
        {936, -417},
        {1085, -56},
        {1090, 354},
        {935, 757},
        {629, 1090},
        {205, 1296},
        {-283, 1331},
        {-766, 1180},
        {-1172, 851},
        {-1435, 384},
        {-1510, -158},
        {-1377, -702},
        {-1049, -1165},
        {-568, -1480},
        {0, -1596},
        {574, -1496},
        {1072, -1191},
        {1422, -724},
        {1576, -165},
        {1515, 406},
        {1251, 908},
        {827, 1273},
        {309, 1453},
        {-226, 1431},
        {-703, 1218},
        {-1058, 856},
        {-1248, 405},
        {-1257, -65},
        {-1100, -489},
        {-810, -810},
        {-441, -992},
        {-53, -1024},
        {297, -916},
        {566, -699},
        {725, -418},
        {765, -121},
        {697, 148},
        {546, 355},
        {348, 479},
        {138, 517},
        {-50, 477},
        {-194, 381},
        {-280, 252},
        {-308, 118},
        {-285, 0},
        {-228, -87},
        {-153, -138},
        {-241, -473},
    }},
};

// USB AM+FM for Wefax (Weather fax RX) , based USB AM with truncated Differentiator band limmited cuttoff 2.400Hz for Audio Tones FM dem. ///////////////////

// IFIR prototype filter: fs=12000, pass=2600, stop=3200, decim=1, fout=12000       // stop band minimum att < -48 dB's (+3300 Hz min atten peak) , rest <50 to -60dB's
constexpr fir_taps_complex<64> taps_2k6_usb_wefax_channel{
    .low_frequency_normalized = 0,
    .high_frequency_normalized = 2600.0f / 12000.0f,
    .transition_normalized = 600.0f / 12000.0f,
    .taps = {{{-14 + 2},
              {-11 - 5},
              {-2 - 8},
              {6 - 5},
              {13 + 1},
              {15 + 14},
              {0 + 26},
              {-22 + 13},
              {-13 - 11},
              {7 - 1},
              {-20 + 17},
              {-47 - 37},
              {33 - 89},
              {122 + 8},
              {19 + 131},
              {-124 + 26},
              {1 - 123},
              {158 + 52},
              {-94 + 245},
              {-363 - 91},
              {36 - 468},
              {524 - 37},
              {67 + 531},
              {-552 + 5},
              {136 - 686},
              {1013 + 258},
              {-204 + 1527},
              {-2104 + 168},
              {-900 - 2529},
              {2577 - 1881},
              {2868 + 2122},
              {-1209 + 3570},
              {-3768 - 52},
              {-1043 - 3412},
              {2634 - 1801},
              {2083 + 1693},
              {-861 + 1927},
              {-1507 - 318},
              {95 - 1041},
              {692 + 100},
              {-189 + 519},
              {-478 - 241},
              {210 - 481},
              {454 + 122},
              {-35 + 372},
              {-262 + 7},
              {4 - 166},
              {116 + 40},
              {-66 + 108},
              {-117 - 62},
              {33 - 117},
              {95 - 2},
              {19 + 57},
              {-23 + 13},
              {3 - 7},
              {6 + 16},
              {-20 + 16},
              {-25 - 9},
              {-8 - 19},
              {4 - 12},
              {7 - 4},
              {7 + 4},
              {2 + 12},
              {-7 + 13}

    }}

};

// WFM 200KF8E emission type //////////////////////////////////////////////

// IFIR image-reject filter: fs=3072000, pass=100000, stop=484000, decim=4, fout=768000
constexpr fir_taps_real<24> taps_200k_wfm_decim_0 = {
    .low_frequency_normalized = -100000.0f / 3072000.0f,
    .high_frequency_normalized = 100000.0f / 3072000.0f,
    .transition_normalized = 384000.0f / 3072000.0f,
    .taps = {{
        48,
        -18,
        -151,
        -364,
        -557,
        -548,
        -139,
        789,
        2187,
        3800,
        5230,
        6071,
        6071,
        5230,
        3800,
        2187,
        789,
        -139,
        -548,
        -557,
        -364,
        -151,
        -18,
        48,
    }},
};

// IFIR prototype filter: fs=768000, pass=100000, stop=284000, decim=2, fout=384000
constexpr fir_taps_real<16> taps_200k_wfm_decim_1 = {
    .low_frequency_normalized = -100000.0f / 768000.0f,
    .high_frequency_normalized = 100000.0f / 768000.0f,
    .transition_normalized = 184000.0f / 768000.0f,
    .taps = {{
        -67,
        -123,
        388,
        622,
        -1342,
        -2185,
        4599,
        14486,
        14486,
        4599,
        -2185,
        -1342,
        622,
        388,
        -123,
        -67,
    }},
};

/* Wideband audio filter */
/* 96kHz int16_t input
 * -> FIR filter, <15kHz (0.156fs) pass, >19kHz (0.198fs) stop
 * -> 48kHz int16_t output, gain of 1.0 (I think).
 * Padded to multiple of four taps for unrolled FIR code.
 * sum(abs(taps)): 125270
 */
constexpr fir_taps_real<64> taps_64_lp_156_198{
    .low_frequency_normalized = -0.156f,
    .high_frequency_normalized = 0.156f,
    .transition_normalized = 0.04f,
    .taps = {{
        -27,
        166,
        104,
        -36,
        -174,
        -129,
        109,
        287,
        148,
        -232,
        -430,
        -130,
        427,
        597,
        49,
        -716,
        -778,
        137,
        1131,
        957,
        -493,
        -1740,
        -1121,
        1167,
        2733,
        1252,
        -2633,
        -4899,
        -1336,
        8210,
        18660,
        23254,
        18660,
        8210,
        -1336,
        -4899,
        -2633,
        1252,
        2733,
        1167,
        -1121,
        -1740,
        -493,
        957,
        1131,
        137,
        -778,
        -716,
        49,
        597,
        427,
        -130,
        -430,
        -232,
        148,
        287,
        109,
        -129,
        -174,
        -36,
        104,
        166,
        -27,
        0,
    }},
};

// WFM 180kHZ General purpose filter with sharp transition , it improves Commercial WFM S/N in weak signals  //////////////////////////////////////////////

// IFIR image-reject filter: fs=3072000, pass=90000, stop=250000, decim=4, fout=768000
constexpr fir_taps_real<24> taps_180k_wfm_decim_0 = {
    .low_frequency_normalized = -90000.0f / 3072000.0f,
    .high_frequency_normalized = 90000.0f / 3072000.0f,
    .transition_normalized = 160000.0f / 3072000.0f,
    .taps = {{
        55,
        122,
        244,
        424,
        666,
        965,
        1308,
        1669,
        2019,
        2321,
        2544,
        2663,
        2663,
        2544,
        2321,
        2019,
        1669,
        1308,
        965,
        666,
        424,
        244,
        122,
        55,
    }},
};

// IFIR prototype filter: fs=768000, pass=90000, stop=170000, decim=2, fout=384000
constexpr fir_taps_real<16> taps_180k_wfm_decim_1 = {
    .low_frequency_normalized = -90000.0f / 768000.0f,
    .high_frequency_normalized = 90000.0f / 768000.0f,
    .transition_normalized = 80000.0f / 768000.0f,
    .taps = {{
        55,
        19,
        -356,
        -916,
        -529,
        2139,
        6695,
        10392,
        10392,
        6695,
        2139,
        -529,
        -916,
        -356,
        19,
        55,
    }},
};

// WFM 80kHZ filter with sharp transition  //////////////////////////////////////////////

// IFIR image-reject filter: fs=3072000, pass=97000, stop=300000, decim=4, fout=768000
constexpr fir_taps_real<24> taps_80k_wfm_decim_0 = {
    .low_frequency_normalized = -97000.0f / 3072000.0f,
    .high_frequency_normalized = 97000.0f / 3072000.0f,
    .transition_normalized = 203000.0f / 3072000.0f,
    .taps = {{
        46,
        112,
        230,
        408,
        650,
        953,
        1301,
        1671,
        2029,
        2340,
        2570,
        2692,
        2692,
        2570,
        2340,
        2029,
        1671,
        1301,
        953,
        650,
        408,
        230,
        112,
        46,
    }},
};

// IFIR prototype filter: fs=768000, pass=37500, stop=112500, decim=2, fout=384000
constexpr fir_taps_real<16> taps_80k_wfm_decim_1 = {
    .low_frequency_normalized = -37500.0f / 768000.0f,
    .high_frequency_normalized = 37500.0f / 768000.0f,
    .transition_normalized = 75000.0f / 768000.0f,
    .taps = {{
        83,
        299,
        743,
        1456,
        2396,
        3418,
        4297,
        4808,
        4808,
        4297,
        3418,
        2396,
        1456,
        743,
        299,
        83,
    }},
};

// WFMAM decimation filters  ////////////////////////////////////////////////
// Used for NOAA 137 Mhz APT sat demod.
// IFIR prototype filter: fs=768000, pass=40000, stop=95000, decim=8, fout=96000
constexpr fir_taps_real<32> taps_80k_wfmam_decim_1 = {
    .low_frequency_normalized = -40000.0f / 768000.0f,
    .high_frequency_normalized = 40000.0f / 768000.0f,
    .transition_normalized = 53000.0f / 768000.0f,
    .taps = {{
        5,
        -37,
        -120,
        -248,
        -397,
        -519,
        -535,
        -354,
        106,
        896,
        2006,
        3355,
        4797,
        6136,
        7171,
        7736,
        7736,
        7171,
        6136,
        4797,
        3355,
        2006,
        896,
        106,
        -354,
        -535,
        -519,
        -397,
        -248,
        -120,
        -37,
        5,

    }},
};

// WFMAM decimation filters  ////////////////////////////////////////////////
// Used for NOAA 137 Mhz APT sat demod.
// IFIR prototype filter: fs=768000, pass=19000, stop=68000, decim=8, fout=96000
constexpr fir_taps_real<32> taps_38k_wfmam_decim_1 = {
    .low_frequency_normalized = -19000.0f / 768000.0f,
    .high_frequency_normalized = 19000.0f / 768000.0f,
    .transition_normalized = 49000.0f / 768000.0f,
    .taps = {{
        49,
        91,
        175,
        303,
        483,
        724,
        1028,
        1391,
        1805,
        2253,
        2712,
        3158,
        3560,
        3891,
        4127,
        4250,
        4250,
        4127,
        3891,
        3560,
        3158,
        2712,
        2253,
        1805,
        1391,
        1028,
        724,
        483,
        303,
        175,
        91,
        49,
    }},
};

/* 1st Wideband FM demod baseband filter of audio AM tones ,
   to pass all DSB band of  AM  fsubcarrier 2.4Khz mod. with APT */
/* 24kHz int16_t input
 * -> FIR filter, <4.5kHz (0.1875fs) pass, >5.2kHz (0.2166fs) stop
 * -> 12kHz int16_t output, gain of 1.0 (I think).
 * sum(abs(taps)): 125152         , before <125270>, very similar.
 */
constexpr fir_taps_real<64> taps_64_lp_1875_2166{
    .low_frequency_normalized = -0.1875f,
    .high_frequency_normalized = 0.1875f,
    .transition_normalized = 0.03f,
    .taps = {{
        38,
        -21,
        -51,
        -9,
        77,
        82,
        -50,
        -168,
        -67,
        190,
        253,
        -61,
        -403,
        -243,
        356,
        616,
        15,
        -814,
        -671,
        550,
        1335,
        334,
        -1527,
        -1689,
        725,
        2978,
        1455,
        -3277,
        -5361,
        830,
        13781,
        24549,
        24549,
        13781,
        830,
        -5361,
        -3277,
        1455,
        2978,
        725,
        -1689,
        -1527,
        334,
        1335,
        550,
        -671,
        -814,
        15,
        616,
        356,
        -243,
        -403,
        -61,
        253,
        190,
        -67,
        -168,
        -50,
        82,
        77,
        -9,
        -51,
        -21,
        38,
    }},
};

/* 1st Wideband FM demod baseband filter of audio AM tones ,
   to pass all DSB band of  AM  fsubcarrier 2.4Khz mod. with APT */
/* 24kHz int16_t input
 * -> FIR filter, BPF center 2k4 carrier ,APT  BW 2kHz
 * -> 12kHz int16_t output, gain of 1.0 (I think).
 */
constexpr fir_taps_real<64> taps_64_bpf_2k4_bw_2k{
    .low_frequency_normalized = -0.1875f,  // not updated, this is just for LPF , waterfall GUI,  we are not using in BPF NOAA app.
    .high_frequency_normalized = 0.1875f,  // not used GUI in NOAA App.
    .transition_normalized = 0.03f,        // not used GUI in NOAA app.
    .taps = {{-45, -29, 32, 63, 0, -125, -181, -81, 61,
              0, -329, -635, -551, -147, 0, -547, -1404, -1625,
              -849, 0, -414, -2118, -3358, -2422, 0, 911, -1792,
              -6126, -6773, 0, 11839, 21131, 21131, 11839, 0, -6773,
              -6126, -1792, 911, 0, -2422, -3358, -2118, -414, 0,
              -849, -1625, -1404, -547, 0, -147, -551, -635, -329,
              0, 61, -81, -181, -125, 0, 63, 32, -29,
              -45}},
};

// TPMS decimation filters ////////////////////////////////////////////////

// IFIR image-reject filter: fs=2457600, pass=100000, stop=407200, decim=4, fout=614400
static constexpr fir_taps_real<24> taps_200k_decim_0 = {
    .low_frequency_normalized = -100000.0f / 2457600.0f,
    .high_frequency_normalized = 100000.0f / 2457600.0f,
    .transition_normalized = 307200.0f / 2457600.0f,
    .taps = {{
        90,
        94,
        4,
        -240,
        -570,
        -776,
        -563,
        309,
        1861,
        3808,
        5618,
        6710,
        6710,
        5618,
        3808,
        1861,
        309,
        -563,
        -776,
        -570,
        -240,
        4,
        94,
        90,
    }},
};

// IFIR prototype filter: fs=614400, pass=100000, stop=207200, decim=2, fout=307200
static constexpr fir_taps_real<16> taps_200k_decim_1 = {
    .low_frequency_normalized = -100000.0f / 614400.0f,
    .high_frequency_normalized = 100000.0f / 614400.0f,
    .transition_normalized = 107200.0f / 614400.0f,
    .taps = {{
        -132,
        -256,
        545,
        834,
        -1507,
        -2401,
        4666,
        14583,
        14583,
        4666,
        -2401,
        -1507,
        834,
        545,
        -256,
        -132,
    }},
};

// BTLE RX decimation filters ////////////////////////////////////////////////
// Default BTLE filter, it is supporting 1M PHY.
// IFIR image-reject filter: fs=4000000, pass=430000, stop=825000, decim=4, fout=1000000
// 1M PHY,  This one it is for the classic bluetooth , (BW = 1Mhz : +-500k, channel space is 2Mhz)
// The traditional transmission of 1 Mbit in the Bluetooth Basic Rate was renamed 1M PHY
static constexpr fir_taps_real<24> taps_BTLE_1M_PHY_decim_0 = {
    .low_frequency_normalized = -430000.0f / 4000000.0f,
    .high_frequency_normalized = 430000.0f / 4000000.0f,
    .transition_normalized = 395000.0f / 4000000.0f,
    .taps = {{

        12,
        57,
        112,
        83,
        -139,
        -531,
        -813,
        -507,
        766,
        2916,
        5255,
        6788,
        6788,
        5255,
        2916,
        766,
        -507,
        -813,
        -531,
        -139,
        83,
        112,
        57,
        12,

    }},
};

// IFIR image-reject filter: fs=4000000, pass=920000, stop=1350000, decim=4, fout=1000000
// Alternative filter, Note : in local test, it improves slightly the sensitivity compared to above filter, but it should have aliasing if co-adjacent channels.
// Then , we leave that filter in the code ,as experimental , but it should not be set up as default one.
// It may work well, in areas where we just receive few signals,  without adjacent channels and weak far away signals.
// 2M PHY , Bluetooth 5 has introduced a new transmission mode with a doubled symbol rate.
// Bluetooth LE has been traditionally transmitting 1 bit per symbol so that theoretically the data rate doubles as well. (BW 2Mhz : +-1Mhz, channel space 2Mhz)
static constexpr fir_taps_real<24> taps_BTLE_2M_PHY_decim_0 = {
    .low_frequency_normalized = -920000.0f / 4000000.0f,
    .high_frequency_normalized = 920000.0f / 4000000.0f,
    .transition_normalized = 430000.0f / 4000000.0f,
    .taps = {{

        -8,
        -20,
        42,
        81,
        -142,
        -234,
        371,
        573,
        -884,
        -1414,
        2573,
        8062,
        8062,
        2573,
        -1414,
        -884,
        573,
        371,
        -234,
        -142,
        81,
        42,
        -20,
        -8,

    }},
};

// Tested to be better at capturing both 4.0 and 5.0 device. Better attenuation at channel end.
static constexpr fir_taps_real<24> taps_BTLE_Dual_PHY = {
    .low_frequency_normalized = -750000.0f / 4000000.0f,
    .high_frequency_normalized = 750000.0f / 4000000.0f,
    .transition_normalized = 250000.0f / 4000000.0f,
    .taps = {{3,
              -5,
              -97,
              -144,
              317,
              1099,
              396,
              -2887,
              -4814,
              1912,
              18134,
              32767,
              32767,
              18134,
              1912,
              -4814,
              -2887,
              396,
              1099,
              317,
              -144,
              -97,
              -5,
              3}},
};

#endif /*__DSP_FIR_TAPS_H__*/
