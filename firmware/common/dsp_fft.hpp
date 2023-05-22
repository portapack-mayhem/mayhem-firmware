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
#include "utility.hpp"
#include "sine_table_int8.hpp"

namespace std {
/* https://github.com/AE9RB/fftbench/blob/master/cxlr.hpp
 * Nice trick from AE9RB (David Turnbull) to get compiler to produce simpler
 * fma (fused multiply-accumulate) instead of worrying about NaN handling
 */
inline complex<float>
operator*(const complex<float>& v1, const complex<float>& v2) {
    return complex<float>{
        v1.real() * v2.real() - v1.imag() * v2.imag(),
        v1.real() * v2.imag() + v1.imag() * v2.real()};
}
} /* namespace std */

template <typename T, size_t N>
void fft_swap(const buffer_c16_t src, std::array<T, N>& dst) {
    static_assert(power_of_two(N), "only defined for N == power of two");

    for (size_t i = 0; i < N; i++) {
        const size_t i_rev = __RBIT(i) >> (32 - log_2(N));
        const auto s = src.p[i];
        dst[i_rev] = {
            static_cast<typename T::value_type>(s.real()),
            static_cast<typename T::value_type>(s.imag())};
    }
}

template <typename T, size_t N>
void fft_swap(const std::array<complex16_t, N>& src, std::array<T, N>& dst) {
    static_assert(power_of_two(N), "only defined for N == power of two");

    for (size_t i = 0; i < N; i++) {
        const size_t i_rev = __RBIT(i) >> (32 - log_2(N));
        const auto s = src[i];
        dst[i_rev] = {
            static_cast<typename T::value_type>(s.real()),
            static_cast<typename T::value_type>(s.imag())};
    }
}

template <typename T, size_t N>
void fft_swap(const std::array<T, N>& src, std::array<T, N>& dst) {
    static_assert(power_of_two(N), "only defined for N == power of two");

    for (size_t i = 0; i < N; i++) {
        const size_t i_rev = __RBIT(i) >> (32 - log_2(N));
        dst[i_rev] = src[i];
    }
}

template <typename T, size_t N>
void fft_swap_in_place(std::array<T, N>& data) {
    static_assert(power_of_two(N), "only defined for N == power of two");

    for (size_t i = 0; i < N / 2; i++) {
        const size_t i_rev = __RBIT(i) >> (32 - log_2(N));
        std::swap(data[i], data[i_rev]);
    }
}

/* http://beige.ucs.indiana.edu/B673/node14.html */
/* http://www.drdobbs.com/cpp/a-simple-and-efficient-fft-implementatio/199500857?pgno=3 */

template <typename T, size_t N>
void fft_c_preswapped(std::array<T, N>& data, const size_t from, const size_t to) {
    static_assert(power_of_two(N), "only defined for N == power of two");
    constexpr auto K = log_2(N);
    if ((to > K) || (from > K)) return;

    constexpr size_t K_max = 8;
    static_assert(K <= K_max, "No FFT twiddle factors for K > 8");
    static constexpr std::array<std::complex<float>, K_max> wp_table{{
        {-2.0f, 0.0f},                                             // 2
        {-1.0f, -1.0f},                                            // 4
        {-0.2928932188134524756f, -0.7071067811865475244f},        // 8
        {-0.076120467488713243872f, -0.38268343236508977173f},     // 16
        {-0.019214719596769550874f, -0.19509032201612826785f},     // 32
        {-0.0048152733278031137552f, -0.098017140329560601994f},   // 64
        {-0.0012045437948276072852f, -0.049067674327418014255f},   // 128
        {-0.00030118130379577988423f, -0.024541228522912288032f},  // 256
    }};

    /* Provide data to this function, pre-swapped. */
    for (size_t k = from; k < to; k++) {
        const size_t mmax = 1 << k;
        const auto wp = wp_table[k];
        T w{1.0f, 0.0f};
        for (size_t m = 0; m < mmax; ++m) {
            for (size_t i = m; i < N; i += mmax * 2) {
                const size_t j = i + mmax;
                const T temp = w * data[j];
                data[j] = data[i] - temp;
                data[i] += temp;
            }
            w += w * wp;
        }
    }
}

/*
   ifft(v,N):
   [0] If N==1 then return.
   [1] For k = 0 to N/2-1, let ve[k] = v[2*k]
   [2] Compute ifft(ve, N/2);
   [3] For k = 0 to N/2-1, let vo[k] = v[2*k+1]
   [4] Compute ifft(vo, N/2);
   [5] For m = 0 to N/2-1, do [6] through [9]
   [6]   Let w.real() = cos(2*PI*m/N)
   [7]   Let w.imag() = sin(2*PI*m/N)
   [8]   Let v[m] = ve[m] + w*vo[m]
   [9]   Let v[m+N/2] = ve[m] - w*vo[m]
 */
template <typename T>
void ifft(T* v, int n, T* tmp) {
    if (n > 1) {
        int k, m;
        T z, w, *vo, *ve;
        ve = tmp;
        vo = tmp + n / 2;
        for (k = 0; k < n / 2; k++) {
            ve[k] = v[2 * k];
            vo[k] = v[2 * k + 1];
        }
        ifft(ve, n / 2, v); /* FFT on even-indexed elements of v[] */
        ifft(vo, n / 2, v); /* FFT on odd-indexed elements of v[] */
        for (m = 0; m < n / 2; m++) {
            w.real(sine_table_i8[((int)(m / (double)n * 0x100 + 0x40)) & 0xFF]);
            w.imag(sine_table_i8[((int)(m / (double)n * 0x100)) & 0xFF]);

            z.real((w.real() * vo[m].real() - w.imag() * vo[m].imag()) / 127); /* Re(w*vo[m]) */
            z.imag((w.real() * vo[m].imag() + w.imag() * vo[m].real()) / 127); /* Im(w*vo[m]) */
            v[m].real(ve[m].real() + z.real());
            v[m].imag(ve[m].imag() + z.imag());
            v[m + n / 2].real(ve[m].real() - z.real());
            v[m + n / 2].imag(ve[m].imag() - z.imag());
        }
    }
    return;
}

#endif /*__DSP_FFT_H__*/
