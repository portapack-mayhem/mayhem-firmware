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

#ifndef _WSPR_PORTAPACK_H_
#define _WSPR_PORTAPACK_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "wspr_lib/wspr_constants.h"

// The receiver bank. WSPR occupies 1400-1600 Hz of the USB audio, laid out in columns of
// one tone spacing: 12000/8192 = 1.46484375 Hz. Column b is a Goertzel at absolute bin
// 956 + b of an 8192-point transform at 12 kHz, so column 0 sits at 1400.39 Hz — rounded
// to the WSPR_RX_BIN0_HZ = 1400 the decoder is told, a quarter bin off the nominal band
// edge and far under the bin width — and the 137 columns cover the protocol band exactly
// (the last column tops out at 1599.2 Hz). The band used to reach 1617.8 Hz with 150
// columns of drift headroom above the band edge; the dual-bank window doubled the matrix,
// and the 13 margin columns were traded for it — they carried only signals already past
// the top of the WSPR band.
#define WSPR_RX_NUM_BINS 137
#define WSPR_RX_NUM_ROWS 176

// Sub-row time oversampling: TWO interleaved Goertzel banks half a symbol apart. Real
// transmissions start at an arbitrary sub-row phase, and a row-quantized decoder misses
// beyond roughly a quarter symbol of smear; with banks closing rows at sample 0 and
// sample 4096 (mod 8192) every phase is within +-2048 samples of one bank's grid. Block-
// row index of the magnitude matrix = 2*symbol_row + bank, so the matrix holds
// 2*176 = 352 rows of WSPR_RX_NUM_BINS bytes.
#define WSPR_RX_TIME_OSR 2
#define WSPR_RX_BANK1_OFFSET (WSPR_SAMPLES_PER_SYMBOL / 2)  // 4096 samples
#define WSPR_RX_BLOCK_ROWS (WSPR_RX_TIME_OSR * WSPR_RX_NUM_ROWS)
#define WSPR_RX_MATRIX_SIZE (WSPR_RX_BLOCK_ROWS * WSPR_RX_NUM_BINS)  // 48224 bytes
#define WSPR_RX_BIN0_HZ 1400

// A transmission is WSPR_NUM_SYMBOLS (162) symbols, about 110.6 s, but the T/R slot is
// 120 s and the receiver cannot know where in the slot the transmission starts, so the
// window is longer than the transmission: 176 rows of 0.6827 s symbols = 120.1 s, leaving
// 14 rows of slack for the slot-tracking loop to move within. The matrix is row-major,
// one uint8 per (block-row, bin), exactly the shape wspr_decode_window() consumes with
// time_osr = 2. Both banks run off the same sample stream: bank 0 closes its rows at
// stream sample 0 (mod 8192), bank 1 starts its first full row at stream sample 4096 and
// then closes every 8192 samples, half a symbol behind bank 0. The window is full when
// bank 0 closes row 175; bank 1's row 175 would end half a symbol past the window, so it
// is never quantized and its bytes stay zero.
typedef struct {
    // Bank-0 rows completed since init/reset: 0..WSPR_RX_NUM_ROWS.
    int rows_filled;
    // Audio samples accumulated into the row currently being built: 0..8191, back to 0
    // the moment a row closes.
    int samples_in_row;
    // Set once row WSPR_RX_NUM_ROWS has been quantized; feed_sample keeps returning true
    // until reset_window.
    bool window_full;
    // Bank-1 counters, the same semantics half a symbol later. bank1_rows_filled only
    // ever reaches WSPR_RX_NUM_ROWS - 1: the last bank-1 row runs past the window end.
    int bank1_rows_filled;
    int bank1_samples_in_row;
    // Total audio samples fed since init/reset; bank 1 begins accumulating at
    // WSPR_RX_BANK1_OFFSET.
    int total_samples;
} wspr_portapack_state_t;

#ifdef __cplusplus
extern "C" {
#endif

// Zeroes the state and builds the coefficient table. The magnitude matrix and the
// Goertzel banks are file-static, so they are cleared here too: a restart of the app must
// not inherit magnitudes from the previous run.
bool wspr_portapack_init(wspr_portapack_state_t* state);

// Nothing is heap-allocated, so this only clears the state.
void wspr_portapack_free(wspr_portapack_state_t* state);

// Feed ONE 12 kHz audio sample. Returns true once the matrix is full (WSPR_RX_NUM_ROWS
// rows), and keeps returning true until reset_window. A 2048-sample DMA chunk delivers
// only about 8 of these; the call is sized for that budget.
bool wspr_portapack_feed_sample(wspr_portapack_state_t* state, float sample);

// Pointers into the static magnitude matrix (uint8, row-major
// (time_osr * rows) x bins, block-row = 2*row + bank). The matrix lives in .bss for the
// lifetime of the app; its contents are only meaningful once window_full is set, and the
// processor calls the decoder on it at that point.
const uint8_t* wspr_portapack_matrix(void);
int wspr_portapack_rows(void);      // WSPR_RX_NUM_ROWS (symbol rows per bank)
int wspr_portapack_time_osr(void);  // WSPR_RX_TIME_OSR (interleaved sub-row banks)
int wspr_portapack_bins(void);      // WSPR_RX_NUM_BINS
int wspr_portapack_bin0_hz(void);   // WSPR_RX_BIN0_HZ

// Clears the matrix and the counters so the next slot starts from an empty window.
void wspr_portapack_reset_window(wspr_portapack_state_t* state);

#ifdef __cplusplus
}
#endif

#endif  // _WSPR_PORTAPACK_H_
