/*
 * Copyright (C) 2022 Belousov Oleg
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

#include <math.h>
#include "dsp_ddc.hpp"

#ifndef M_PI
#define M_PI (3.14159265358979323846264338327950288)
#endif

namespace dsp {

void DDC::set_sample_rate(float x) {
	sample_rate = x;

	osc_vect_i = 0;
	osc_vect_q = 1;
}

void DDC::set_freq(const float x) {
	double rate = (2 * M_PI * x) / sample_rate;
	
	osc_sin = sin(rate);
	osc_cos = cos(rate);

	osc_vect_i = 0;
	osc_vect_q = 1;
}

buffer_c16_t DDC::execute(const buffer_c16_t& src, const buffer_c16_t& dst) {
	auto src_p = src.p;
	auto dst_p = dst.p;

	for (size_t count = 0; count < src.count; count++) {
		// Oscillator
	
		double osc_i = (osc_vect_i * osc_cos) + (osc_vect_q * osc_sin);
		double osc_q = (osc_vect_q * osc_cos) - (osc_vect_i * osc_sin);

		osc_vect_q = osc_q;
		osc_vect_i = osc_i;
	
		// DDC
	
		double i = src_p->real();
		double q = src_p->imag();
		
		dst_p->real((i * osc_q) + (q * osc_i));
		dst_p->imag((q * osc_q) - (i * osc_i));
		
		src_p++;
		dst_p++;
	}

	double gain = (3.0f - ((osc_vect_q * osc_vect_q) + (osc_vect_i * osc_vect_i))) / 2.0f;

	osc_vect_q *= gain;
	osc_vect_i *= gain;

	return { dst.p, src.count, src.sampling_rate };
}

}
