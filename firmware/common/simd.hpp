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

#ifndef __SIMD_H__
#define __SIMD_H__

#if defined(LPC43XX_M4)

#include <hal.h>

#include <cstdint>

struct vec4_s8 {
	union {
		int8_t v[4];
		uint32_t w;
	};
};

struct vec2_s16 {
	constexpr vec2_s16(
	) : v { 0, 0 }
	{
	}

	constexpr vec2_s16(
		const int16_t v
	) : v { v, v }
	{
	}
	
	constexpr vec2_s16(
		const int16_t v0,
		const int16_t v1
	) : v { v0, v1 }
	{
	}

	constexpr vec2_s16(
		const vec2_s16& other
	) : w { other.w }
	{
	}

	vec2_s16& operator=(const vec2_s16& other) {
		w = other.w;
		return *this;
	}

	union {
		int16_t v[2];
		uint32_t w;
	};
};

static inline vec4_s8 rev16(const vec4_s8 v) {
	vec4_s8 result;
	result.w = __REV16(v.w);
	return result;
}

static inline vec4_s8 pkhbt(const vec4_s8 v1, const vec4_s8 v2, const size_t sh = 0) {
	vec4_s8 result;
	result.w = __PKHBT(v1.w, v2.w, sh);
	return result;
}

static inline vec2_s16 pkhbt(const vec2_s16 v1, const vec2_s16 v2, const size_t sh = 0) {
	vec2_s16 result;
	result.w = __PKHBT(v1.w, v2.w, sh);
	return result;
}

static inline vec2_s16 pkhtb(const vec2_s16 v1, const vec2_s16 v2, const size_t sh = 0) {
	vec2_s16 result;
	result.w = __PKHTB(v1.w, v2.w, sh);
	return result;
}

static inline vec2_s16 sxtb16(const vec4_s8 v, const size_t sh = 0) {
	vec2_s16 result;
	result.w = __SXTB16(v.w, sh);
	return result;
}

static inline int32_t smlsd(const vec2_s16 v1, const vec2_s16 v2, const int32_t accum) {
	return __SMLSD(v1.w, v2.w, accum);
}

static inline int32_t smlad(const vec2_s16 v1, const vec2_s16 v2, const int32_t accum) {
	return __SMLAD(v1.w, v2.w, accum);
}

#endif /* defined(LPC43XX_M4) */

#endif/*__SIMD_H__*/
