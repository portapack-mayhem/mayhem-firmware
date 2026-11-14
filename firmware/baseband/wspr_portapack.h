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
// edge and far under the bin width — and the 150 columns reach 1617.8 Hz, leaving margin
// above the 1600 Hz band edge for a signal that drifts.
#define WSPR_RX_NUM_BINS 150
#define WSPR_RX_NUM_ROWS 176
#define WSPR_RX_MATRIX_SIZE (WSPR_RX_NUM_ROWS * WSPR_RX_NUM_BINS)  // 26400 bytes
#define WSPR_RX_BIN0_HZ 1400

// A transmission is WSPR_NUM_SYMBOLS (162) symbols, about 110.6 s, but the T/R slot is
// 120 s and the receiver cannot know where in the slot the transmission starts, so the
// window is longer than the transmission: 176 rows of 0.6827 s symbols = 120.1 s, leaving
// 14 rows of slack for the slot-tracking loop to move within. The matrix is row-major,
// one uint8 per (row, bin), exactly the shape wspr_decode_window() consumes.
typedef struct {
    // Rows completed since init/reset: 0..WSPR_RX_NUM_ROWS.
    int rows_filled;
    // Audio samples accumulated into the row currently being built: 0..8191, back to 0
    // the moment a row closes.
    int samples_in_row;
    // Set once row WSPR_RX_NUM_ROWS has been quantized; feed_sample keeps returning true
    // until reset_window.
    bool window_full;
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

// Pointers into the static magnitude matrix (uint8, row-major rows x bins). The matrix
// lives in .bss for the lifetime of the app; its contents are only meaningful once
// window_full is set, and the processor calls the decoder on it at that point.
const uint8_t* wspr_portapack_matrix(void);
int wspr_portapack_rows(void);     // WSPR_RX_NUM_ROWS
int wspr_portapack_bins(void);     // WSPR_RX_NUM_BINS
int wspr_portapack_bin0_hz(void);  // WSPR_RX_BIN0_HZ

// Clears the matrix and the counters so the next slot starts from an empty window.
void wspr_portapack_reset_window(wspr_portapack_state_t* state);

#ifdef __cplusplus
}
#endif

#endif  // _WSPR_PORTAPACK_H_
