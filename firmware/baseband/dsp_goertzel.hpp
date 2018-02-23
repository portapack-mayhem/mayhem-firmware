/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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

#ifndef __DSP_GOERTZEL_H__
#define __DSP_GOERTZEL_H__

#include "dsp_types.hpp"

namespace dsp {

class GoertzelDetector {
public:
	GoertzelDetector(const float frequency, const uint32_t sample_rate);
	
	float execute(const buffer_s16_t& src);

private:
	float coefficient { };
	int16_t s[2] { 0 };
};

} /* namespace dsp */

#endif/*__DSP_GOERTZEL_H__*/
