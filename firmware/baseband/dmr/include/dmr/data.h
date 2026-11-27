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

#ifndef DMR_DATA_H
#define DMR_DATA_H

#include <dmr/burst.h>
#include <stdint.h>

enum {
    DMR_DATA_TYPE_VOICE_LC_HEADER = 1,
    DMR_DATA_TYPE_TERMINATOR_LC = 2
};

/* Decode the 8-bit CC/Data Type field protected by Golay(20,8). */
int dmr_slot_type_decode(uint32_t received, unsigned int* color_code, unsigned int* data_type, unsigned int* corrected_bits);

/* Extract the information bits from the interleaved BPTC payload.
 * This implementation does not correct BPTC errors; Full LC verifies RS parity. */
int dmr_bptc_196_96_decode(const uint8_t encoded[25], uint8_t information[12]);

/* Decode Voice Header/Terminator LC and validate masked RS(12,9) parity. */
int dmr_full_lc_decode(const dmr_burst_t* burst, uint8_t lc[9], unsigned int* color_code, unsigned int* data_type);

#endif
