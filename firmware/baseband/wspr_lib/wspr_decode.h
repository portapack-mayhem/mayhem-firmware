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

#ifndef _WSPR_DECODE_H_
#define _WSPR_DECODE_H_

#include <stdint.h>

#include "wspr_constants.h"

/*
 * WSPR decoder, host-first: it works on a magnitude matrix of exactly the
 * shape the baseband receiver produces, so the same code runs in host tests
 * and in firmware. The matrix holds WSPR_NUM_SYMBOLS rows of num_bins bytes,
 * row-major: mag[row * num_bins + col]. Column spacing must be exactly one
 * tone step (12000/8192 = 1.4648 Hz), like the FT8 receiver keeps its bins
 * aligned to the tone spacing.
 *
 * The decode has no CRC to lean on: a candidate is accepted when its payload
 * unpacks into a valid standard message.
 */

#define WSPR_MAX_RESULTS 4

typedef struct {
    char callsign[13];   // up to 12 characters plus terminator
    char grid[5];        // 4 characters plus terminator
    int8_t dbm;          // 0..60
    int8_t snr_db;       // rough estimate, for display only
    int16_t freq_hz;     // audio frequency of tone 0, relative to the dial
    int16_t drift_bins;  // total frequency walk over the window, in bins
    int16_t row_offset;  // window row the transmission starts in; the slot
                         // tracker steers this toward the middle of the slack
    int16_t sync_score;  // raw sync-sum of this candidate, for display: a
                         // quiet band scores a fraction of a noisy-signal one
    int16_t metric;      // final sequential-decode path metric: real signals
                         // accumulate a clear positive score, noise hovers
                         // around zero - the decode-acceptance gate reads this
} wspr_decode_result_t;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Try to decode one capture window. mag: num_rows * num_bins uint8
 * magnitudes, row-major, with num_rows >= WSPR_NUM_SYMBOLS (a 120 s slot
 * capture has 176 rows for the 162-symbol transmission). bin0_hz: audio
 * frequency of column 0. Returns the number of results written.
 */
int wspr_decode_window(const uint8_t* mag,
                       int num_rows,
                       int num_bins,
                       int bin0_hz,
                       wspr_decode_result_t* results,
                       int max_results);

#ifdef __cplusplus
}
#endif

#endif  // _WSPR_DECODE_H_
