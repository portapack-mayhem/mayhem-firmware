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

#include <dmr/burst.h>

#include <string.h>

int dmr_burst_set_dibit(
    uint8_t data[DMR_BURST_BYTE_COUNT],
    size_t index,
    uint8_t dibit) {
    if (data == NULL || index >= DMR_BURST_DIBIT_COUNT || dibit > 3u)
        return 0;

    const unsigned int shift = 6u - 2u * (unsigned int)(index & 3u);
    const uint8_t mask = (uint8_t)(3u << shift);

    data[index / 4u] = (uint8_t)((data[index / 4u] & ~mask) |
                                 (dibit << shift));
    return 1;
}

uint8_t dmr_burst_get_bit(
    const uint8_t data[DMR_BURST_BYTE_COUNT],
    size_t index) {
    if (data == NULL || index >= DMR_BURST_BIT_COUNT)
        return UINT8_MAX;

    return (uint8_t)((data[index / 8u] >> (7u - index % 8u)) & 1u);
}
