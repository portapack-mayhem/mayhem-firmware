/*
 * Copyright (C) 2026 Belousov Oleg aka R1CBU
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

#ifndef DMR_PHY_SAMPLE_RING_H
#define DMR_PHY_SAMPLE_RING_H

#include <stdint.h>

enum { DMR_RX_SAMPLE_RING_CAPACITY = 640 };

/* Exact uint64 index % 640 using only 32-bit arithmetic. ARMv7-M has no
 * 64-bit divide: the original expression called __aeabi_uldivmod twice
 * for every interpolated symbol, thousands of times per sync burst.
 * 2^32 % 640 = 256, and 5*256 % 640 = 0. Intermediate sum <= 1663. */

static inline uint32_t dmr_sample_ring_index(uint64_t index) {
    const uint32_t lo = (uint32_t)index;
    const uint32_t hi = (uint32_t)(index >> 32);

    return (lo % 640u + (hi % 5u) * 256u) % 640u;
}

/* Offsets come only from bounded finite timing/drift searches. Truncation
 * plus one correction is floor, without the libm call on Cortex-M4. */

static inline int dmr_sample_offset_floor(float offset) {
    const int whole = (int)offset;
    return (float)whole > offset ? whole - 1 : whole;
}
#endif
