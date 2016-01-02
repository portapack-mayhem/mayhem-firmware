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

#ifndef __DSP_IIR_CONFIG_H__
#define __DSP_IIR_CONFIG_H__

#include "dsp_iir.hpp"

// scipy.signal.butter(2, 300 / 24000.0, 'highpass', analog=False)
constexpr iir_biquad_config_t audio_hpf_300hz_config {
	{  0.97261390f, -1.94522780f,  0.97261390f, },
	{  1.00000000f, -1.94447766f,  0.94597794f, }
};

// scipy.signal.iirdesign(wp=8000 / 24000.0, ws= 4000 / 24000.0, gpass=1, gstop=18, ftype='ellip')
constexpr iir_biquad_config_t non_audio_hpf_config {
	{  0.51891061f, -0.95714180f,  0.51891061f },
	{  1.0f       , -0.79878302f,  0.43960231f }

// scipy.signal.butter(1, 300 / 24000.0, 'lowpass', analog=False)
// NOTE: Technically, order-1 filter, b[2] = a[2] = 0.
constexpr iir_biquad_config_t audio_deemph_300_6_config {
	{  0.01925927f,  0.01925927f,  0.00000000f, },
	{  1.00000000f, -0.96148145f,  0.00000000f, },
};

#endif/*__DSP_IIR_CONFIG_H__*/
