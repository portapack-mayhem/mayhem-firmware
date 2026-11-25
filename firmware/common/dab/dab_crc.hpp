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

/* FIB CRC-16 and DAB+ superframe firecode. Bitwise: a few runs per frame do
 * not justify a 512-byte table in the M4's tight code region. */

#ifndef __DAB_CRC_H__
#define __DAB_CRC_H__

#include <cstddef>
#include <cstdint>

namespace dab {

/* Poly 0x1021, init 0xFFFF, no final inversion; "123456789" -> 0x29B1. */
uint16_t crc16_ccitt(const uint8_t* data, size_t length);

/* Covers the 30 payload bytes; stored inverted, big-endian, in bytes 30-31. */
bool fib_crc_ok(const uint8_t fib[32]);
void fib_crc_stamp(uint8_t fib[32]);

/* (16,11) shortened Fire code, generator 0x782F, over superframe bytes 2..10,
 * stored in bytes 0..1. */
bool firecode_ok(const uint8_t superframe_head[11]);
void firecode_stamp(uint8_t superframe_head[11]);

} /* namespace dab */

#endif /*__DAB_CRC_H__*/
