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

#ifndef _WSPR_CONSTANTS_H_
#define _WSPR_CONSTANTS_H_

#include <stdint.h>

/*
 * WSPR protocol constants. References:
 *   - "The WSPR Coding Process", Andy Talbot G4JNT, 2009 (translated to HTML
 *     at https://swharden.com/software/FSKview/wspr)
 *   - http://ganymedeham.blogspot.com/2015/04/arduino-wspr-symbol-generator.html
 *     (independent copy of the sync vector, cross-checked bit by bit)
 *   - https://hackaday.io/project/166875-careless-wspr/log/167301-encoding-wsprs
 *
 * The sync vector below matched both of the first two sources bit for bit
 * (162 entries, 63 ones).
 */

#define WSPR_SAMPLE_RATE 12000
// One 4-FSK symbol is 8192 samples: tone spacing = 12000/8192 = 1.46484375 Hz.
#define WSPR_SAMPLES_PER_SYMBOL 8192
#define WSPR_NUM_SYMBOLS 162
// 162 symbols at 0.682667 s each: a transmission lasts about 110.6 s.
#define WSPR_SYMBOL_SECONDS_NUMERATOR 8192
#define WSPR_SYMBOL_SECONDS_DENOMINATOR 12000

// Message payload: 28-bit callsign + 15-bit locator + 5-bit power + 2-bit type.
#define WSPR_PAYLOAD_BITS 50
#define WSPR_CALLSIGN_BITS 28
#define WSPR_LOCATOR_BITS 15
// The power field is 7 bits of which the top bit marks a standard message;
// Wikipedia splits the same field into 5 power bits + 2 type bits.
#define WSPR_POWER_FIELD_BITS 7

// Rate 1/2, constraint length K = 32, non-recursive convolutional code.
// nsym = (50 + K - 1) * 2 = 162 coded bits; 31 zero tail bits flush the state.
#define WSPR_CONSTRAINT_LENGTH 32
#define WSPR_TAIL_BITS (WSPR_CONSTRAINT_LENGTH - 1)
#define WSPR_SOURCE_BITS (WSPR_PAYLOAD_BITS + WSPR_TAIL_BITS)
#define WSPR_CODED_BITS 162
#define WSPR_SOURCE_BYTES ((WSPR_SOURCE_BITS + 7) / 8)  // 11

// Generator polynomials (G4JNT / Hackaday), also used as the initial shift
// register states by the documented encoding process.
#define WSPR_POLY_0 0xF2D05351u
#define WSPR_POLY_1 0xE4613C47u

// WSPR occupies 1400-1600 Hz of the USB audio; decode windows are laid out in
// bins of one tone spacing. First analysed bin, bin spacing and count are set
// by the receiver, not here.

#ifdef __cplusplus
extern "C" {
#endif

// The 162-bit sync vector; symbol[n] = sync[n] + 2 * data[n], sync is the LSB.
extern const uint8_t wspr_sync_vector[WSPR_CODED_BITS];

#ifdef __cplusplus
}
#endif

#endif  // _WSPR_CONSTANTS_H_
