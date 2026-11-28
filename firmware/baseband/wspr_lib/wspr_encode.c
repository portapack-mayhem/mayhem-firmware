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

#include "wspr_encode.h"

#include <string.h>

/*
 * Everything below follows "The WSPR Coding Process" (G4JNT 2009). Formulas
 * quoted there, kept verbatim where useful:
 *
 * Callsign, char values '0'-'9' = 0..9, 'A'-'Z' = 10..35, space = 36:
 *   N1 = [Ch 1]
 *   N2 = N1 * 36 + [Ch 2]
 *   N3 = N2 * 10 + [Ch 3]
 *   N4 = 27 * N3 + [Ch 4] - 10
 *   N5 = 27 * N4 + [Ch 5] - 10
 *   N6 = 27 * N5 + [Ch 6] - 10
 * (max 37*36*10*27*27*27 - 1 = 262177559 < 2^28)
 *
 * Locator, [L1..L2] = letter - 'A' (0..17), [L3..L4] = digit (0..9):
 *   M1 = (179 - 10 * [L1] - [L3]) * 180 + 10 * [L2] + [L4]
 * (AA00 -> 32220, RR99 -> 179; note the reversed direction, as documented)
 *
 * Power, 7-bit field with the message-type marker in the top bits:
 *   M = M1 * 128 + [Pwr] + 64
 *
 * 50-bit payload = N (28 bits) followed by M (22 bits), MSB-first.
 */


static uint8_t bit_reverse_8(uint8_t v) {
    v = (uint8_t)(((v & 0x55) << 1) | ((v >> 1) & 0x55));
    v = (uint8_t)(((v & 0x33) << 2) | ((v >> 2) & 0x33));
    v = (uint8_t)(((v & 0x0F) << 4) | ((v >> 4) & 0x0F));
    return v;
}

static uint8_t parity32(uint32_t v) {
    v ^= v >> 16;
    v ^= v >> 8;
    v ^= v >> 4;
    v ^= v >> 2;
    v ^= v >> 1;
    return (uint8_t)(v & 1u);
}

void wspr_interleave(const uint8_t s[WSPR_CODED_BITS], uint8_t d[WSPR_CODED_BITS]) {
    // Exactly as the G4JNT document describes: walk the 8-bit bit-reversal of
    // every address 0..255, keep the ones below 162, and feed the source bits
    // in natural order through a counter that only advances on kept entries.
    // Bit reversal restricted to 162 entries is NOT its own inverse because of
    // that compaction, which is why wspr_deinterleave exists.
    int p = 0;
    for (unsigned i = 0; i < 256; i++) {
        const unsigned j = bit_reverse_8((uint8_t)i);
        if (j < WSPR_CODED_BITS) {
            d[j] = s[p++];
        }
    }
}

void wspr_deinterleave(const uint8_t d[WSPR_CODED_BITS], uint8_t s[WSPR_CODED_BITS]) {
    // Inverse of wspr_interleave: same walk, reading the source back out.
    int p = 0;
    for (unsigned i = 0; i < 256; i++) {
        const unsigned j = bit_reverse_8((uint8_t)i);
        if (j < WSPR_CODED_BITS) {
            s[p++] = d[j];
        }
    }
}

void wspr_encode_bits(const uint8_t bits[WSPR_SOURCE_BYTES], uint8_t coded[WSPR_CODED_BITS]) {
    uint32_t reg0 = 0;
    uint32_t reg1 = 0;
    uint8_t s[WSPR_CODED_BITS];

    // Convolve: both registers shift the same source bit in from the LSB; the
    // output bits are the parities of register & polynomial. The registers
    // start at zero; the polynomials are tap masks, not initial states. The
    // G4JNT wording ("Register 0: 0xF2D05351") reads either way, so this was
    // pinned empirically: synthetic audio of this encoder decodes with the
    // canonical wsprd decoder, the preloaded variant does not.
    for (int i = 0; i < WSPR_SOURCE_BITS; i++) {
        const uint8_t b = (uint8_t)((bits[i >> 3] >> (7 - (i & 7))) & 1u);
        reg0 = (reg0 << 1) | (uint32_t)b;
        reg1 = (reg1 << 1) | (uint32_t)b;
        s[2 * i] = parity32(reg0 & WSPR_POLY_0);
        s[2 * i + 1] = parity32(reg1 & WSPR_POLY_1);
    }

    wspr_interleave(s, coded);
}
