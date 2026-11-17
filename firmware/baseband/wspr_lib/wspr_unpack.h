/*
 * Copyright (C) 2026 Ivan
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

#ifndef _WSPR_UNPACK_H_
#define _WSPR_UNPACK_H_

#include <stdint.h>
#include <stdbool.h>

/*
 * Inverse of the documented 50-bit packing: the first 28 bits are the
 * callsign, the last 22 bits carry the 15-bit locator and the 7-bit power
 * field whose top bit marks a standard (type 1) message. Compound-callsign
 * and 6-digit-locator messages (types 2 and 3) are not handled: they fail
 * the type marker check.
 */

#define WSPR_CALLSIGN_MAX 13  // 12 characters plus terminator
#define WSPR_GRID_LEN 4

typedef struct {
    char callsign[WSPR_CALLSIGN_MAX];
    char grid[WSPR_GRID_LEN + 1];
    int8_t dbm;
} wspr_message_t;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Unpack the first 50 bits of bits[] (MSB-first bytes) into a message.
 * Returns false when the payload is not a valid standard message: wrong type
 * marker, locator out of range, or a callsign that does not survive the
 * structural rules (a digit in third position, letters/spaces after).
 */
bool wspr_unpack_message(const uint8_t* bits, wspr_message_t* message);

#ifdef __cplusplus
}
#endif

#endif  // _WSPR_UNPACK_H_
