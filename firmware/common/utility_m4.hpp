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

#ifndef __UTILITY_M4_H__
#define __UTILITY_M4_H__

#if defined(LPC43XX_M4)

#include <hal.h>

static inline complex32_t multiply_conjugate_s16_s32(const complex16_t::rep_type a, const complex16_t::rep_type b) {
	// conjugate: conj(a + bj) = a - bj
	// multiply: (a + bj) * (c + dj) = (ac - bd) + (bc + ad)j
	// conjugate-multiply: (ac + bd) + (bc - ad)j
	//return { a.real() * b.real() + a.imag() * b.imag(), a.imag() * b.real() - a.real() * b.imag() };
	// NOTE: Did not use combination of SMUAD and SMUSDX because of non-saturating arithmetic.
	// const int32_t r = __SMUAD(a, b);
	// const int32_t i = __SMUSDX(b, a);
	const int32_t rr = __SMULBB(a, b);
	const int32_t ii = __SMULTT(a, b);
	const int32_t r = __QADD(rr, ii);
	const int32_t ir = __SMULTB(a, b);
	const int32_t ri = __SMULBT(a, b);
	const int32_t i = __QSUB(ir, ri);
	return { r, i };
}
#endif /* defined(LPC43XX_M4) */

#endif/*__UTILITY_M4_H__*/
