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

#ifdef WSPR_ENCODE_TEST_API
static int char_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    if (c == ' ' || c == '\0') return 36;
    return -1;
}

bool wspr_pack_message(const char* callsign, const char* locator, int dbm, uint8_t bits[WSPR_SOURCE_BYTES]) {
    char six[7];
    uint32_t n = 0;
    size_t len;
    int i, j;

    memset(bits, 0, WSPR_SOURCE_BYTES);
    if (callsign == NULL || locator == NULL) return false;
    if (dbm < 0 || dbm > 60) return false;

    len = strlen(callsign);
    if (len == 0 || len > 6) return false;

    // Pad the call to six characters first, so validation below never reads
    // past the string.
    memset(six, ' ', 6);
    six[6] = '\0';
    memcpy(six, callsign, len);

    // Third character must be a digit; one leading space is the only repair,
    // which needs the call itself to fit in five characters.
    if (char_value(six[2]) > 9) {
        if (len >= 6) return false;
        memmove(six + 1, six, 5);
        six[0] = ' ';
    }
    if (char_value(six[2]) < 0 || char_value(six[2]) > 9) return false;

    // Character set and positions: first two any of 0-9/A-Z/space, a digit in
    // third position, letters or spaces after. Once the padding starts,
    // nothing but spaces may follow.
    for (i = 0; i < 2; i++) {
        if (char_value(six[i]) < 0) return false;
    }
    for (i = 3; i < 6; i++) {
        const int v = char_value(six[i]);
        if (v < 10 || v > 36) return false;
        if (v == 36) {
            for (j = i + 1; j < 6; j++) {
                if (char_value(six[j]) != 36) return false;
            }
        }
    }

    {
        const int c1 = char_value(six[0]);
        const int c2 = char_value(six[1]);
        const int c3 = char_value(six[2]);
        const int c4 = char_value(six[3]);
        const int c5 = char_value(six[4]);
        const int c6 = char_value(six[5]);
        n = (uint32_t)c1;
        n = n * 36 + (uint32_t)c2;
        n = n * 10 + (uint32_t)c3;
        n = n * 27 + (uint32_t)(c4 - 10);
        n = n * 27 + (uint32_t)(c5 - 10);
        n = n * 27 + (uint32_t)(c6 - 10);
    }
    if (n > 0x0FFFFFFFu) return false;

    if (strlen(locator) != 4) return false;
    {
        const int l1 = locator[0];
        const int l2 = locator[1];
        const int d1 = locator[2];
        const int d2 = locator[3];
        if (l1 < 'A' || l1 > 'R' || l2 < 'A' || l2 > 'R') return false;
        if (d1 < '0' || d1 > '9' || d2 < '0' || d2 > '9') return false;
        const int l1v = l1 - 'A';
        const int l2v = l2 - 'A';
        const int d1v = d1 - '0';
        const int d2v = d2 - '0';
        const uint32_t m1 = (uint32_t)((179 - 10 * l1v - d1v) * 180 + 10 * l2v + d2v);
        const uint32_t m = m1 * 128u + (uint32_t)dbm + 64u;
        // 28 + 22 bits: needs 64 bits, a 32-bit shift would drop the call.
        const uint64_t payload = ((uint64_t)n << 22) | (uint64_t)m;

        // MSB-first: bit k of the payload lands in byte k/8, bit 7-(k%8).
        for (i = 0; i < WSPR_PAYLOAD_BITS; i++) {
            if ((payload >> (WSPR_PAYLOAD_BITS - 1 - i)) & 1u) {
                bits[i >> 3] |= (uint8_t)(0x80u >> (i & 7));
            }
        }
    }
    return true;
}
#endif  // WSPR_ENCODE_TEST_API

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

#ifdef WSPR_ENCODE_TEST_API
void wspr_merge_sync(const uint8_t coded[WSPR_CODED_BITS], uint8_t tones[WSPR_CODED_BITS]) {
    int i;
    for (i = 0; i < WSPR_CODED_BITS; i++) {
        tones[i] = (uint8_t)(wspr_sync_vector[i] + 2 * coded[i]);
    }
}

bool wspr_encode_message(const char* callsign, const char* locator, int dbm, uint8_t tones[WSPR_CODED_BITS]) {
    uint8_t bits[WSPR_SOURCE_BYTES];
    uint8_t coded[WSPR_CODED_BITS];

    if (!wspr_pack_message(callsign, locator, dbm, bits)) return false;
    wspr_encode_bits(bits, coded);
    wspr_merge_sync(coded, tones);
    return true;
}
#endif  // WSPR_ENCODE_TEST_API
