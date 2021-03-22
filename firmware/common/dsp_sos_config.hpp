/*
 * Copyright (C) 2020 Belousov Oleg
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

#ifndef __DSP_SOS_CONFIG_H__
#define __DSP_SOS_CONFIG_H__

#include "dsp_iir.hpp"

// scipy.signal.iirfilter(ftype="ellip", N = 10, rp = 0.5, rs = 60.0, Wn = 0.5, btype = 'lowpass', output="sos")

constexpr iir_biquad_df2_config_t half_band_lpf_config[5] = {
	{ 0.02339042f,	0.0411599f,		0.02339042f,	1.0f,		-0.95317621f,	0.33446485f },
	{ 1.0f,			0.82196114f,	1.0f,			1.0f,       -0.50327735f,	0.63611027f },
	{ 1.0f,         0.32515305f,	1.0f,           1.0f,       -0.18144446f,   0.85269598f },
	{ 1.0f,         0.14394122f,	1.0f,          	1.0f,       -0.04368236f,	0.94798064f },
	{ 1.0f,         0.08720754,		1.0f,           1.0f,        0.00220944f,  	0.98743139f }
};

#endif/*__DSP_SOS_CONFIG_H__*/
