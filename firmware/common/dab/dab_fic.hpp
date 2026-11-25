/*
 * Copyright (C) 2026 Mayhem firmware contributors
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

/* FIC depuncturing (EN 300 401 section 11.2): 768 payload bits (three FIBs)
 * plus a 6-bit tail give 3096 mother bits, punctured to 2304, three symbols:
 * 21 blocks x 4 x 24 (PI_16) + 3 x 4 x 23 (PI_15) + 12 of the 24-bit tail. */

#ifndef __DAB_FIC_H__
#define __DAB_FIC_H__

#include <cstddef>
#include <cstdint>

namespace dab {

constexpr size_t kFicPayloadBits = 768;

/* 4 * (768 + 6) */
constexpr size_t kFicMotherBits = 3096;

constexpr size_t kFicPuncturedBits = 2304;

/* 12 FIBs per frame in groups of 3. */
constexpr size_t kFicCodewordsPerFrame = 4;

/* Writes 0 (erasure) where a bit was removed; returns mother bits written. */
size_t depuncture_fic(const int8_t* punctured, int8_t* out, size_t out_capacity);

/* Into the packed trellis input (dab_soft.hpp), one step word per four mother
 * bits. Returns mother bits written. */
size_t depuncture_fic_packed(const int8_t* punctured, uint16_t* out, size_t out_capacity_steps);

} /* namespace dab */

#endif /*__DAB_FIC_H__*/
