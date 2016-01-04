/*
 * Copyright (C) 2013 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __DSP_FFT_H__
#define __DSP_FFT_H__

#include <cstdint>
#include <cstddef>
#include <complex>
#include <cmath>
#include <type_traits>
#include <array>

#include "dsp_types.hpp"
#include "complex.hpp"
#include "hal.h"

namespace std {
	/* https://github.com/AE9RB/fftbench/blob/master/cxlr.hpp
	 * Nice trick from AE9RB (David Turnbull) to get compiler to produce simpler
	 * fma (fused multiply-accumulate) instead of worrying about NaN handling
	 */
	inline complex<float>
	operator*(const complex<float>& v1, const complex<float>& v2) {
		return complex<float> {
			v1.real() * v2.real() - v1.imag() * v2.imag(),
			v1.real() * v2.imag() + v1.imag() * v2.real()
		};
	}
} /* namespace std */

constexpr bool power_of_two(const size_t n) {
	return (n & (n - 1)) == 0;
}

constexpr size_t log_2(const size_t n, const size_t p = 0) {
	return (n <= 1) ? p : log_2(n / 2, p + 1);
}

template<typename T, size_t N>
void fft_swap(const buffer_c16_t src, std::array<T, N>& dst) {
	static_assert(power_of_two(N), "only defined for N == power of two");

	for(size_t i=0; i<N; i++) {
		const size_t i_rev = __RBIT(i) >> (32 - log_2(N));
		const auto s = src.p[i];
		dst[i_rev] = {
			static_cast<typename T::value_type>(s.real()),
			static_cast<typename T::value_type>(s.imag())
		};
	}
}

template<typename T, size_t N>
void fft_swap(const std::array<complex16_t, N>& src, std::array<T, N>& dst) {
	static_assert(power_of_two(N), "only defined for N == power of two");

	for(size_t i=0; i<N; i++) {
		const size_t i_rev = __RBIT(i) >> (32 - log_2(N));
		const auto s = src[i];
		dst[i_rev] = {
			static_cast<typename T::value_type>(s.real()),
			static_cast<typename T::value_type>(s.imag())
		};
	}
}

template<typename T, size_t N>
void fft_swap(const std::array<T, N>& src, std::array<T, N>& dst) {
	static_assert(power_of_two(N), "only defined for N == power of two");

	for(size_t i=0; i<N; i++) {
		const size_t i_rev = __RBIT(i) >> (32 - log_2(N));
		dst[i_rev] = src[i];
	}
}

template<typename T, size_t N>
void fft_swap_in_place(std::array<T, N>& data) {
	static_assert(power_of_two(N), "only defined for N == power of two");

	for(size_t i=0; i<N/2; i++) {
		const size_t i_rev = __RBIT(i) >> (32 - log_2(N));
		std::swap(data[i], data[i_rev]);
	}
}

/* http://beige.ucs.indiana.edu/B673/node14.html */
/* http://www.drdobbs.com/cpp/a-simple-and-efficient-fft-implementatio/199500857?pgno=3 */

template<typename T, size_t N>
void fft_c_preswapped(std::array<T, N>& data) {
	static_assert(power_of_two(N), "only defined for N == power of two");
	constexpr auto K = log_2(N);

	constexpr size_t K_max = 8;
	static_assert(K <= K_max, "No FFT twiddle factors for K > 8");
	static constexpr std::array<std::complex<float>, K_max> wp_table { {
		{ -2.00000000000000000000000000000000f,  0.00000000000000000000000000000000f },
		{ -1.00000000000000000000000000000000f, -1.00000000000000000000000000000000f },
		{ -0.29289321881345242726268907063059f, -0.70710678118654746171500846685376f },
		{ -0.07612046748871323376128827931097f, -0.38268343236508978177923268049199f },
		{ -0.01921471959676954860407604996908f, -0.19509032201612824808378832130984f },
		{ -0.00481527332780311376897453001789f, -0.09801714032956060362877792613290f },
		{ -0.00120454379482760713659939000308f, -0.04906767432741801493456534899451f },
		{ -0.00030118130379577984830768988544f, -0.02454122852291228812360301958506f },
	} };

	/* Provide data to this function, pre-swapped. */
	for(size_t k = 0; k < log_2(N); k++) {
		const size_t mmax = 1 << k;
		const auto wp = wp_table[k];
		T w { 1.0f, 0.0f };
		for(size_t m = 0; m < mmax; ++m) {
			for(size_t i = m; i < N; i += mmax * 2) {
				const size_t j = i + mmax;
				const T temp = w * data[j];
				data[j]  = data[i] - temp;
				data[i] += temp;
			}
			w += w * wp;
		}
	}
}

#endif/*__DSP_FFT_H__*/
