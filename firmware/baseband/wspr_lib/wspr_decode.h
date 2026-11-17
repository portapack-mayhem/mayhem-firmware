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
 * and in firmware. The matrix holds num_rows rows of num_bins bytes, row-
 * major: mag[row * num_bins + col]. Column spacing must be exactly one tone
 * step (12000/8192 = 1.4648 Hz), like the FT8 receiver keeps its bins
 * aligned to the tone spacing.
 *
 * time_osr extends the rows into interleaved sub-row banks: block-row index
 * = time_osr * row + bank, so mag holds time_osr * num_rows rows and the
 * address of (row, col) in bank b is mag[(time_osr * row + b) * num_bins +
 * col]. time_osr = 1 is the plain single-grid matrix; time_osr = 2 is the
 * dual-bank window the receiver produces, two Goertzel grids half a symbol
 * apart, so a transmission at an arbitrary sub-row phase is never more than
 * a quarter symbol away from one bank's symbol boundaries. Stage 1 and
 * stage 2 run per bank on their own candidate lists and the results merge
 * through the shared duplicate/frequency suppression; row_offset stays in
 * symbol rows and the bank that produced the result is reported separately.
 *
 * The decode has no CRC to lean on: a candidate is accepted when its payload
 * unpacks into a valid standard message.
 */

#define WSPR_MAX_RESULTS 4

typedef struct {
    char callsign[13];     // up to 12 characters plus terminator
    char grid[5];          // 4 characters plus terminator
    int8_t dbm;            // 0..60
    int8_t snr_db;         // rough estimate, for display only
    int16_t freq_hz;       // audio frequency of tone 0, relative to the dial
    int16_t drift_bins;    // total frequency walk over the window, in bins
    int16_t row_offset;    // window row the transmission starts in; the slot
                           // tracker steers this toward the middle of the slack
    int16_t sync_score;    // raw sync-sum of this candidate, for display: a
                           // quiet band scores a fraction of a noisy-signal one
    int16_t metric;        // final sequential-decode path metric: real signals
                           // accumulate a clear positive score, noise hovers
                           // around zero - the decode-acceptance gate reads this
    int8_t bank;           // sub-row bank of the decode, 0..time_osr-1; the
                           // row_offset is a symbol row of that bank's grid
    int8_t agreement_pct;  // re-encode agreement: how many of the received
                           // symbols match the payload re-encoded back to
                           // channel symbols; real decodes land near 100,
                           // impostors near 50 - the impostor gate reads this
    int8_t agg_windows;    // 0 = plain per-window decode; >= 2 = aggregated
                           // decode over that many accumulated windows (the
                           // multi-slot weight integration below)
} wspr_decode_result_t;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Try to decode one capture window. mag: time_osr * num_rows * num_bins
 * uint8 magnitudes, row-major, with num_rows >= WSPR_NUM_SYMBOLS (a 120 s
 * slot capture has 176 rows for the 162-symbol transmission). time_osr: 1
 * (single row grid) or 2 (two interleaved sub-row banks). bin0_hz: audio
 * frequency of column 0. Returns the number of results written.
 */
int wspr_decode_window(const uint8_t* mag,
                       int num_rows,
                       int num_bins,
                       int bin0_hz,
                       wspr_decode_result_t* results,
                       int max_results,
                       int time_osr);

/*
 * Clear the multi-slot accumulation table. A WSPR station repeats the same
 * message in successive 2-minute slots, and the decoder integrates the soft
 * weights of accepted decodes across those slots in a file-static cluster
 * table; this resets it. The receiver calls it once per app start so a
 * restart never inherits clusters from the previous run.
 */
void wspr_decode_reset_aggregation(void);

#ifdef __cplusplus
}
#endif

#endif  // _WSPR_DECODE_H_
