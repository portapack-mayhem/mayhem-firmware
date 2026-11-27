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

#ifndef _WSPR_ENCODE_H_
#define _WSPR_ENCODE_H_

#include <stdint.h>
#include <stdbool.h>

#include "wspr_constants.h"

/*
 * Reference WSPR encoder, written from the G4JNT "Coding Process" document.
 * The decoder depends on its FEC core (wspr_encode_bits, wspr_interleave and
 * wspr_deinterleave) at runtime, and those are linked into firmware. The
 * fixture/message-generation entry points (wspr_pack_message, wspr_merge_sync,
 * wspr_encode_message) exist to generate synthetic test signals on the host
 * and are compiled only when WSPR_ENCODE_TEST_API is defined. Keeping them
 * here makes the decoder tests self-contained.
 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Pack a standard (type 1) message "CALL LOC4 dBm" into 50 payload bits,
 * MSB-first in bits[0..WSPR_SOURCE_BYTES), followed by the zero tail.
 *
 * Callsign rules: up to 6 characters, third character must be a digit (a
 * single leading space is added when it is not), last three characters are
 * letters or spaces. Locator: 4 characters, first two A-R, last two digits.
 * dbm: 0..60.
 *
 * Returns false for anything that does not fit the standard message (compound
 * callsigns and 6-digit locators are type 2/3, which are not handled).
 */
#ifdef WSPR_ENCODE_TEST_API
bool wspr_pack_message(const char* callsign, const char* locator, int dbm, uint8_t bits[WSPR_SOURCE_BYTES]);
#endif

// The documented bit-reversal interleave (G4JNT): walk addresses 0..255,
// keep bit-reversed destinations below 162, feed source bits through a
// counter advancing only on kept entries. The compaction breaks self-inverse
// symmetry, so the decoder needs the separate wspr_deinterleave.
void wspr_interleave(const uint8_t s[WSPR_CODED_BITS], uint8_t d[WSPR_CODED_BITS]);

// Inverse of wspr_interleave; this is the direction the decoder uses.
void wspr_deinterleave(const uint8_t d[WSPR_CODED_BITS], uint8_t s[WSPR_CODED_BITS]);

// Encode 81 source bits (50 payload + 31 zero tail, MSB-first) into the 162
// interleaved coded bits.
void wspr_encode_bits(const uint8_t bits[WSPR_SOURCE_BYTES], uint8_t coded[WSPR_CODED_BITS]);

// Merge the coded bits with the sync vector: tone = sync + 2 * data.
#ifdef WSPR_ENCODE_TEST_API
void wspr_merge_sync(const uint8_t coded[WSPR_CODED_BITS], uint8_t tones[WSPR_CODED_BITS]);
#endif

// Convenience: pack + encode + merge. Returns false when packing failed.
#ifdef WSPR_ENCODE_TEST_API
bool wspr_encode_message(const char* callsign, const char* locator, int dbm, uint8_t tones[WSPR_CODED_BITS]);
#endif

#ifdef __cplusplus
}
#endif

#endif  // _WSPR_ENCODE_H_
