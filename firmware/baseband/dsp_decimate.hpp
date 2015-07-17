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

#ifndef __DSP_DECIMATE_H__
#define __DSP_DECIMATE_H__

#include <cstdint>
#include <array>

#include "dsp_types.hpp"

namespace dsp {
namespace decimate {

class TranslateByFSOver4AndDecimateBy2CIC3 {
public:
	buffer_c16_t execute(
		buffer_c8_t src,
		buffer_c16_t dst
	);

private:
	uint32_t _q1_i0 { 0 };
	uint32_t _q0_i1 { 0 };
};

class DecimateBy2CIC3 {
public:
	buffer_c16_t execute(
		buffer_c16_t src,
		buffer_c16_t dst
	);

private:
	uint32_t _iq0 { 0 };
	uint32_t _iq1 { 0 };
};

class FIR64AndDecimateBy2Real {
public:
	static constexpr size_t taps_count = 64;

	FIR64AndDecimateBy2Real(
		const std::array<int16_t, taps_count>& taps
	) : taps(taps)
	{
	}

	buffer_s16_t execute(
		buffer_s16_t src,
		buffer_s16_t dst
	);

private:
	std::array<int16_t, taps_count + 2> z;
	const std::array<int16_t, taps_count>& taps;
};

size_t fir_and_decimate_by_2_complex(
	const complex16_t* const src_start,
	const size_t src_count,
	complex16_t* const dst_start,
	complex16_t* const z,
	const complex16_t* const taps,
	const size_t taps_count
);

size_t fir_and_decimate_by_2_complex_fast(
	const complex16_t* const src_start,
	const size_t src_count,
	complex16_t* const dst_start,
	complex16_t* const z,
	const complex16_t* const taps,
	const size_t taps_count
);

template<size_t taps_count>
class FIRAndDecimateBy2Complex {
public:
	/* NOTE! Current code makes an assumption that block of samples to be
	 * processed will be a multiple of the taps_count.
	 */
	FIRAndDecimateBy2Complex(
		const std::array<int16_t, taps_count>& real_taps
	) {
		for(size_t i=0; i<taps_count; i++) {
			taps[             i] = real_taps[i];
			taps[taps_count + i] = real_taps[i];
		}
	}

	buffer_c16_t execute(
		buffer_c16_t src,
		buffer_c16_t dst
	) {
		const auto dst_count = fir_and_decimate_by_2_complex_fast(src.p, src.count, dst.p, z.data(), taps.data(), taps_count);
		return { dst.p, dst_count, src.sampling_rate / 2 };
	}

private:
	std::array<complex16_t, taps_count * 2> taps;
	std::array<complex16_t, taps_count> z;
};

class DecimateBy2CIC4Real {
public:
	buffer_s16_t execute(
		buffer_s16_t src,
		buffer_s16_t dst
	);

private:
	int16_t z[5];
};
#if 0
class DecimateBy2HBF5Complex {
public:
	buffer_c16_t execute(
		buffer_c16_t const src,
		buffer_c16_t const dst
	);

private:
	complex16_t z[11];
};

class DecimateBy2HBF7Complex {
public:
	buffer_c16_t execute(
		buffer_c16_t const src,
		buffer_c16_t const dst
	);

private:
	complex16_t z[11];
};
#endif
/* From http://www.dspguru.com/book/export/html/3

Here are several basic techniques to fake circular buffers:

Split the calculation: You can split any FIR calculation into its "pre-wrap"
and "post-wrap" parts. By splitting the calculation into these two parts, you
essentially can do the circular logic only once, rather than once per tap.
(See fir_double_z in FirAlgs.c above.)

Duplicate the delay line: For a FIR with N taps, use a delay line of size 2N.
Copy each sample to its proper location, as well as at location-plus-N.
Therefore, the FIR calculation's MAC loop can be done on a flat buffer of N
points, starting anywhere within the first set of N points. The second set of
N delayed samples provides the "wrap around" comparable to a true circular
buffer. (See fir_double_z in FirAlgs.c above.)

Duplicate the coefficients: This is similar to the above, except that the
duplication occurs in terms of the coefficients, not the delay line.
Compared to the previous method, this has a calculation advantage of not
having to store each incoming sample twice, and it also has a memory
advantage when the same coefficient set will be used on multiple delay lines.
(See fir_double_h in FirAlgs.c above.)

Use block processing: In block processing, you use a delay line which is a
multiple of the number of taps. You therefore only have to move the data
once per block to implement the delay-line mechanism. When the block size
becomes "large", the overhead of a moving the delay line once per block
becomes negligible.
*/

#if 0
template<size_t N>
class FIRAndDecimateBy2Complex {
public:
	FIR64AndDecimateBy2Complex(
		const std::array<int16_t, N>& taps
	) : taps { taps }
	{
	}

	buffer_c16_t execute(
		buffer_c16_t const src,
		buffer_c16_t const dst
	) {
		/* int16_t input (sample count "n" must be multiple of 4)
		 * -> int16_t output, decimated by 2.
		 * taps are normalized to 1 << 16 == 1.0.
		 */

		return { dst.p, src.count / 2 };
	}

private:
	std::array<complex16_t, N> z;
	const std::array<int16_t, N>& taps;

	complex<int16_t> process_one(const size_t start_offset) {
		const auto split = &z[start_offset];
		const auto end = &z[z.size()];
		auto tap = &taps[0];

		complex<int32_t> t { 0, 0 };

		auto p = split;
		while(p < end) {
			const auto t = *(tap++);
			const auto c = *(p++);
			t.real += c.real * t;
			t.imag += c.imag * t;
		}

		p = &z[0];
		while(p < split) {
			const auto t = *(tap++);
			const auto c = *(p++);
			t.real += c.real * t;
			t.imag += c.imag * t;
		}

		return { t.real / 65536, t.imag / 65536 };
	}
};
#endif
} /* namespace decimate */
} /* namespace dsp */

#endif/*__DSP_DECIMATE_H__*/
