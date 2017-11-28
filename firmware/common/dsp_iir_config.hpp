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

#ifndef __DSP_IIR_CONFIG_H__
#define __DSP_IIR_CONFIG_H__

#include "dsp_iir.hpp"

// scipy.signal.butter(2, 30 / 24000.0, 'highpass', analog=False)
constexpr iir_biquad_config_t audio_48k_hpf_30hz_config {
	{  0.99722705f, -1.99445410f,  0.99722705f },
	{  1.00000000f, -1.99444641f,  0.99446179f }
};

// scipy.signal.butter(2, 300 / 24000.0, 'highpass', analog=False)
constexpr iir_biquad_config_t audio_48k_hpf_300hz_config {
	{  0.97261390f, -1.94522780f,  0.97261390f },
	{  1.00000000f, -1.94447766f,  0.94597794f }
};

// scipy.signal.butter(2, 300 / 12000.0, 'highpass', analog=False)
constexpr iir_biquad_config_t audio_24k_hpf_300hz_config {
	{  0.94597686f, -1.89195371f,  0.94597686f },
	{  1.00000000f, -1.88903308f,  0.89487434f }

};

// scipy.signal.butter(2, 30 / 12000.0, 'highpass', analog=False)
constexpr iir_biquad_config_t audio_24k_hpf_30hz_config {
	{  0.99446179f, -1.98892358f,  0.99446179f },
	{  1.00000000f, -1.98889291f,  0.98895425f }
};

// scipy.signal.butter(2, 300 / 8000.0, 'highpass', analog=False)
constexpr iir_biquad_config_t audio_16k_hpf_300hz_config {
	{  0.92006616f, -1.84013232f,  0.92006616f },
	{  1.00000000f, -1.83373266f,  0.84653197f }
};

// scipy.signal.butter(2, 300 / 6000.0, 'highpass', analog=False)
constexpr iir_biquad_config_t audio_12k_hpf_300hz_config {
	{  0.89485861f, -1.78971721f,  0.89485861f },
	{  1.00000000f, -1.77863178f,  0.80080265f }
};

// scipy.signal.butter(2, 300 / 4000.0, 'highpass', analog=False)
constexpr iir_biquad_config_t audio_8k_hpf_300hz_config {
	{  0.84645925f, -1.69291851f,  0.84645925f },
	{  1.00000000f, -1.66920314f,  0.71663387f }
};

// scipy.signal.iirdesign(wp=8000 / 24000.0, ws= 4000 / 24000.0, gpass=1, gstop=18, ftype='ellip')
constexpr iir_biquad_config_t non_audio_hpf_config {
	{  0.51891061f, -0.95714180f,  0.51891061f },
	{  1.0f       , -0.79878302f,  0.43960231f }
};

// scipy.signal.butter(1, 300 / 24000.0, 'lowpass', analog=False)
// NOTE: Technically, order-1 filter, b[2] = a[2] = 0.
constexpr iir_biquad_config_t audio_48k_deemph_300_6_config {
	{  0.01925927f,  0.01925927f,  0.00000000f },
	{  1.00000000f, -0.96148145f,  0.00000000f }
};

// scipy.signal.butter(1, 300 / 12000.0, 'lowpass', analog=False)
// NOTE: Technically, order-1 filter, b[2] = a[2] = 0.
constexpr iir_biquad_config_t audio_24k_deemph_300_6_config {
	{  0.03780475f,  0.03780475f,  0.00000000f },
	{  1.00000000f, -0.92439049f,  0.00000000f }
};

// scipy.signal.butter(1, 300 / 8000.0, 'lowpass', analog=False)
// NOTE: Technically, order-1 filter, b[2] = a[2] = 0.
constexpr iir_biquad_config_t audio_16k_deemph_300_6_config {
	{  0.05568894f,  0.05568894f,  0.00000000f },
	{  1.00000000f, -0.88862213f,  0.00000000f }
};

// scipy.signal.butter(1, 300 / 6000.0, 'lowpass', analog=False)
// NOTE: Technically, order-1 filter, b[2] = a[2] = 0.
constexpr iir_biquad_config_t audio_12k_deemph_300_6_config {
	{  0.07295966f,  0.07295966f,  0.00000000f },
	{  1.00000000f, -0.85408069f,  0.00000000f }
};

// scipy.signal.butter(1, 300 / 4000.0, 'lowpass', analog=False)
// NOTE: Technically, order-1 filter, b[2] = a[2] = 0.
constexpr iir_biquad_config_t audio_8k_deemph_300_6_config {
	{  0.10583178f,  0.10583178f,  0.00000000f },
	{  1.00000000f, -0.78833643f,  0.00000000f }
};

// 75us RC time constant, used in broadcast FM in Americas, South Korea
// scipy.signal.butter(1, 2122 / 24000.0, 'lowpass', analog=False)
// NOTE: Technically, order-1 filter, b[2] = a[2] = 0.
constexpr iir_biquad_config_t audio_48k_deemph_2122_6_config {
	{  0.12264116f,  0.12264116f,  0.00000000f },
	{  1.00000000f, -0.75471767f,  0.00000000f }
};

#endif/*__DSP_IIR_CONFIG_H__*/
