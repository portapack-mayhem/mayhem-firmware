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

#ifndef __UTILITY_H__
#define __UTILITY_H__

#include <type_traits>
#include <cstdint>
#include <cstddef>
#include <complex>
#include <memory>

#include <hal.h>

#define LOCATE_IN_RAM __attribute__((section(".ramtext")))

constexpr size_t operator "" _KiB(unsigned long long v) {
	return v * 1024;
}

constexpr size_t operator "" _MiB(unsigned long long v) {
	return v * 1024 * 1024;
}

template<typename E>
constexpr typename std::underlying_type<E>::type toUType(E enumerator) noexcept {
	/* Thanks, Scott Meyers! */
	return static_cast<typename std::underlying_type<E>::type>(enumerator);
}

inline uint32_t flp2(uint32_t v) {
	v |= v >>  1;
	v |= v >>  2;
	v |= v >>  4;
	v |= v >>  8;
	v |= v >> 16;
	return v - (v >> 1);
}

uint32_t gcd(const uint32_t u, const uint32_t v);

template<class T>
inline constexpr T pow(const T base, unsigned const exponent) {
	return (exponent == 0) ? 1 : (base * pow(base, exponent - 1));
}

#if defined(LPC43XX_M4)
static inline bool m4_flag_saturation() {
	return __get_APSR() & (1U << 27);
}

static inline void clear_m4_flag_saturation() {
	uint32_t flags = 1;
	__asm volatile ("MSR APSR_nzcvqg, %0" : : "r" (flags));
}

#endif

float complex16_mag_squared_to_dbv_norm(const float c16_mag_squared);

inline float magnitude_squared(const std::complex<float> c) {
	const auto r = c.real();
	const auto r2 = r * r;
	const auto i = c.imag();
	const auto i2 = i * i;
	return r2 + i2;
}

/* Override new/delete to use Chibi/OS heap functions */
/* NOTE: Do not inline these, it doesn't work. ;-) */
void* operator new(size_t size);
void* operator new[](size_t size);
void operator delete(void* p);
void operator delete[](void* p);

namespace std {

/*! Stephan T Lavavej (STL!) implementation of make_unique, which has been accepted into the C++14 standard.
 * It includes handling for arrays.
 * Paper here: http://isocpp.org/files/papers/N3656.txt
 */
template<class T> struct _Unique_if {
	typedef unique_ptr<T> _Single_object;
};

//! Specialization for unbound array
template<class T> struct _Unique_if<T[]> {
	typedef unique_ptr<T[]> _Unknown_bound;
};

//! Specialization for array of known size
template<class T, size_t N> struct _Unique_if<T[N]> {
	typedef void _Known_bound;
};

//! Specialization for normal object type
template<class T, class... Args>
typename _Unique_if<T>::_Single_object
make_unique(Args&& ... args) {
	return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

//! Specialization for unknown bound
template<class T>
typename _Unique_if<T>::_Unknown_bound
make_unique(size_t n) {
	typedef typename remove_extent<T>::type U;
	return unique_ptr<T>(new U[n]());
}

//! Deleted specialization
template<class T, class... Args>
typename _Unique_if<T>::_Known_bound
make_unique(Args&& ...) = delete;

} /* namespace std */

#endif/*__UTILITY_H__*/
