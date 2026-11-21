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
// (the last column tops out at 1599.2 Hz).
#define WSPR_RX_NUM_BINS 137
#define WSPR_RX_NUM_ROWS 176

// Sub-row time oversampling: TWO interleaved Goertzel banks half a symbol apart. Real
// transmissions start at an arbitrary sub-row phase, and a row-quantized decoder misses
// beyond roughly a quarter symbol of smear; with banks closing rows at sample 0 and
// sample 4096 (mod 8192) every phase is within +-2048 samples of one bank's grid.
#ifndef WSPR_RX_TIME_OSR
#define WSPR_RX_TIME_OSR 2
#endif
#if WSPR_RX_TIME_OSR != 1 && WSPR_RX_TIME_OSR != 2
#error "WSPR_RX_TIME_OSR must be 1 or 2"
#endif
#define WSPR_RX_BANK1_OFFSET (WSPR_SAMPLES_PER_SYMBOL / 2)  // 4096 samples
#define WSPR_RX_BLOCK_ROWS (WSPR_RX_TIME_OSR * WSPR_RX_NUM_ROWS)
#define WSPR_RX_MATRIX_SIZE (WSPR_RX_BLOCK_ROWS * WSPR_RX_NUM_BINS)  // 48224 bytes
#define WSPR_RX_BIN0_HZ 1400

/*
 * Ring capture (ring architecture, stage 3).
 *
 * The capture never stops and never resets: the 352-row magnitude buffer is a RING.
 * Block-rows complete strictly alternately — bank 0's row r closes at stream sample
 * (r+1)*8192, bank 1's row r at WSPR_RX_BANK1_OFFSET + (r+1)*8192 — so the completion
 * order is br 0 = bank 0 row 0, br 1 = bank 1 row 0, br 2 = bank 0 row 1, ... The
 * ABSOLUTE block-row id is the ring counter itself: bank = parity (even = bank 0),
 * symbol row = br / 2, and the physical slot of block-row br is br % WSPR_RX_BLOCK_ROWS.
 * A new row overwrites the slot of block-row br - 352, so the ring always holds the
 * last 352 block-rows (120.1 s) of the stream.
 *
 * Publication protocol (single M4 core, decode thread on the same core):
 *   - feed_sample writes the row's 137 bytes into the physical slot FIRST, then
 *     increments state->total_br LAST. total_br is volatile, which orders the compiler:
 *     no byte store can sink below the counter store. The baseband thread (writer) and
 *     the decode thread (reader) share the core and its unified SRAM, so the reader,
 *     after observing total_br > br, observes every byte of row br. The physical slot
 *     of a published row is not rewritten until 352 rows later (~120 s), which is the
 *     scan/snapshot deadline the ring scan enforces (see wspr_ring_scan_poll).
 *   - uint32 wrap of total_br (~46 years of block-rows at 12 kHz): every consumer must
 *     derive the physical slot as id % 352 on the SAME raw uint32 ids the producer
 *     uses, and compare ids only through the signed difference (int32_t)(a - b) — the
 *     one wrap-safe arithmetic near 2^32. Because 2^32 is NOT a multiple of 352, the
 *     slot sequence teleports once at the wrap (from slot 2^32 % 352 - 1 to 0): rows
 *     written in the ~224 slots preceding the wrap have a reduced overwrite margin
 *     (down to ~128 rows) for that one cycle, so a packet anchored within ~100
 *     block-rows before the wrap cannot complete. Once in 46 years; the ring scan's
 *     dedup/expire counters simply ride through it. The near-wrap host tests
 *     exercise the id arithmetic and a packet anchored just after the wrap.
 */
typedef struct {
    // Completed block-rows since init: the ring counter and the absolute id of the
    // most recently completed row (id = total_br - 1). Published after the row bytes.
    volatile uint32_t total_br;
    // Audio samples accumulated into the row currently being built, per bank.
    uint32_t bank0_samples_in_row;
    uint32_t bank1_samples_in_row;
    // Total audio samples fed since init (12 kHz, wraps after ~99.4 h); telemetry only.
    uint32_t total_samples;
    // One-shot: bank 1 begins accumulating at stream sample WSPR_RX_BANK1_OFFSET, once
    // per session. The flag (not a comparison against total_samples) is what keeps the
    // half-symbol offset from re-arming when total_samples itself wraps.
    bool bank1_started;
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

// Feed ONE 12 kHz audio sample. Returns true when this sample COMPLETED a block-row
// (its bytes are published: total_br has been incremented past it). The caller wakes the
// decode thread on true. A 2048-sample DMA chunk delivers only about 8 of these; the
// call is sized for that budget.
bool wspr_portapack_feed_sample(wspr_portapack_state_t* state, float sample);

// Absolute block-row counter: the ids of all completed rows are 0 .. total_br() - 1.
uint32_t wspr_portapack_total_br(const wspr_portapack_state_t* state);

// Rows currently held intact by the ring: min(total_br(), WSPR_RX_BLOCK_ROWS).
uint32_t wspr_portapack_valid_rows(const wspr_portapack_state_t* state);

// Pointer to the physical slot of absolute block-row br (uint32-wrap-safe). Valid once
// total_br() > br; the bytes stay intact until total_br() > br + WSPR_RX_BLOCK_ROWS.
const uint8_t* wspr_portapack_row_br(uint32_t br);

// Base address of the ring (physical slot 0) and its geometry, for the ring scan and
// the ring snapshot reader.
const uint8_t* wspr_portapack_matrix(void);
int wspr_portapack_rows(void);      // WSPR_RX_NUM_ROWS (symbol rows per bank)
int wspr_portapack_time_osr(void);  // WSPR_RX_TIME_OSR (interleaved sub-row banks)
int wspr_portapack_bins(void);      // WSPR_RX_NUM_BINS
int wspr_portapack_bin0_hz(void);   // WSPR_RX_BIN0_HZ

#ifdef __cplusplus
}
#endif

#endif  // _WSPR_PORTAPACK_H_
