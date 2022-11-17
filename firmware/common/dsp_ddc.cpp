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

#include "dsp_ddc.hpp"
#include "cordic.hpp"

namespace dsp {

void DDC::set_sample_rate(const int32_t x) {
	sample_rate = x;
}

void DDC::set_freq(const int32_t x) {
	phase_inc = (2.0 * CORDIC_PI * x) / sample_rate;
}

buffer_c16_t DDC::execute(const buffer_c16_t& src, const buffer_c16_t& dst) {
	auto src_p = src.p;
	auto dst_p = dst.p;

	for (size_t count = 0; count < src.count; count++) {
		int32_t i = src_p->real() * 0.607252935 * 16384;
		int32_t q = src_p->imag() * 0.607252935 * 16384;
		
		cordic(phase, &i, &q);
		
		dst_p->real(i / 16384);
		dst_p->imag(q / 16384);
		
		phase += phase_inc;

		if (phase < -CORDIC_PI) {
			phase += CORDIC_PI * 2;
		} else if (phase > CORDIC_PI) {
			phase -= CORDIC_PI * 2;
		}
		
		src_p++;
		dst_p++;
	}

	return { dst.p, src.count, src.sampling_rate };
}

}
