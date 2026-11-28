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

#include "wspr_portapack.h"

#include "wspr_lib/wspr_fast_math.h"
#include "wspr_lib/wspr_decode.h"  // WSPR_RING_COMPILER_BARRIER (publication protocol)

#include <string.h>

// Column b of the matrix is absolute bin WSPR_RX_FIRST_BIN + b of the 8192-point
// transform; bin 956 is the integer bin nearest the 1400 Hz band edge
// (956 * 12000/8192 = 1400.39 Hz).
#define WSPR_RX_FIRST_BIN 956

// The ring matrix and the banks are file-static in .bss: 48224 matrix bytes (352
// block-rows, two interleaved banks) plus 3 x 600 bytes of shared coefficient bank and
// 2 x (2 x 600) bytes of per-bank state, 55.4 KB — the same footprint the old window
// matrix had. The ring never resets it: rows are simply overwritten in slot order.
static uint8_t magnitude[WSPR_RX_MATRIX_SIZE];
static float goertzel_coeff[WSPR_RX_NUM_BINS];
static float goertzel_s1[WSPR_RX_BANKS][WSPR_RX_NUM_BINS];
static float goertzel_s2[WSPR_RX_BANKS][WSPR_RX_NUM_BINS];

// cos_unit_quadrant() and log2_from_bits() are the shared libm-free helpers in
// wspr_lib/wspr_fast_math.h. For this coefficient table the highest bin is 1105, giving
// 2*pi*1105/8192 = 0.85 rad, and the cosine error stays within 1.2e-12 across
// all 150 bins. cosf()/log10f() are avoided because their newlib argument
// reduction / log tables cost 2,304 bytes of the 32 KB app budget; log10f
// would also push the row-closing DMA chunk from about 100 us past 200 us of
// the 666.7 us budget (see the header's comments).

// Quantize one row's Goertzel power to the byte scale the decoder reads
// (byte = 20*log10(power/N^2) + 240, the scale the FT8 waterfall uses and the wspr
// decoder tests are calibrated to: a byte step is 0.5 dB).
//
// The Goertzel sum for an on-bin tone of amplitude A is A*N/2, so dividing the power by
// N*N puts a full-scale tone at A^2/4, byte 228. Dividing by N instead runs the scale
// 39.1 dB hot at N = 8192: a tone above roughly 0.02 of full scale pins at 255 together
// with its neighbour bins, and the decoder only ever compares bytes against their
// neighbours, so the scale would collapse. There is no AGC ahead of this stage.
//
// The row lands in the physical slot of absolute block-row `br`: slot = br % 352, which
// by the strict completion alternation (even br = bank 0, odd = bank 1) is exactly the
// row-major block-row index of the interleaved layout the decoder has always walked.
static void finish_row(wspr_portapack_state_t* state, int bank) {
    // The caller already knows the id: feed_sample closes rows in strict order, so the
    // row being finished here IS block-row total_br (bank 0 and bank 1 alternate and
    // never close in the same sample: their boundaries sit half a symbol apart).
    const uint32_t br_id = state->total_br;
    uint8_t* const slot = magnitude + (size_t)(br_id % (uint32_t)WSPR_RX_BLOCK_ROWS) * WSPR_RX_NUM_BINS;

    for (int b = 0; b < WSPR_RX_NUM_BINS; b++) {
        float s1 = goertzel_s1[bank][b];
        float s2 = goertzel_s2[bank][b];
        float power = s1 * s1 + s2 * s2 - goertzel_coeff[b] * s1 * s2;
        power *= (1.0f / ((float)WSPR_SAMPLES_PER_SYMBOL * (float)WSPR_SAMPLES_PER_SYMBOL));
        if (power < 1e-20f) power = 1e-20f;

        // 20/log2(10) = 6.0206 converts the log2 above into the byte scale.
        int m = (int)(6.0206f * log2_from_bits(power) + 240.0f);
        if (m < 0) m = 0;
        if (m > 255) m = 255;
        slot[b] = (uint8_t)m;
    }

    // Publication: the counter moves ONLY after the row bytes are in place (see the
    // ownership protocol in wspr_portapack.h and wspr_decode.h). The compiler barrier
    // stops the byte stores from sinking below the counter store; on the single-core
    // M4 a context switch preserves program order, so the volatile counter + barrier
    // is the whole protocol (single-core ordering only - NOT a C11 data-race-free
    // claim, and no protection against another core or DMA writing these rows).
    WSPR_RING_COMPILER_BARRIER();
    state->total_br = br_id + 1u;
    WSPR_RING_COMPILER_BARRIER();

    memset(goertzel_s1[bank], 0, sizeof(goertzel_s1[bank]));
    memset(goertzel_s2[bank], 0, sizeof(goertzel_s2[bank]));
    if (bank) {
        state->bank1_samples_in_row = 0;
    } else {
        state->bank0_samples_in_row = 0;
    }
}

bool wspr_portapack_init(wspr_portapack_state_t* state) {
    if (!state) return false;

    memset((void*)state, 0, sizeof(*state));

    // The table, the banks and the matrix are file-static, so a restart of the app has
    // to clear them. This is the ONLY matrix clearing in the ring: there is no per-window
    // reset any more, rows are overwritten in slot order.
    const float two_pi_over_n = 2.0f * 3.14159265358979f / (float)WSPR_SAMPLES_PER_SYMBOL;
    for (int b = 0; b < WSPR_RX_NUM_BINS; b++) {
        const int k = WSPR_RX_FIRST_BIN + b;
        goertzel_coeff[b] = 2.0f * cos_unit_quadrant(two_pi_over_n * k);
    }

    memset(magnitude, 0, sizeof(magnitude));
    memset(goertzel_s1, 0, sizeof(goertzel_s1));
    memset(goertzel_s2, 0, sizeof(goertzel_s2));

    return true;
}

void wspr_portapack_free(wspr_portapack_state_t* state) {
    if (!state) return;
    // Nothing to release: the matrix and the banks are file-static. The state is cleared
    // so a stale pointer cannot keep reporting completed rows.
    memset((void*)state, 0, sizeof(*state));
}

// One Goertzel pass over all bins for one bank. About 12 instructions per bin, the same
// loop shape and -Os count as the FT8 bank.
static void goertzel_pass(float sample, float* s1, float* s2) {
    for (int b = 0; b < WSPR_RX_NUM_BINS; b++) {
        const float s0 = sample + goertzel_coeff[b] * s1[b] - s2[b];
        s2[b] = s1[b];
        s1[b] = s0;
    }
}

bool wspr_portapack_feed_sample(wspr_portapack_state_t* state, float sample) {
    if (!state) return false;

    // Inf passes the NaN check, since Inf == Inf holds.
    if (sample != sample || sample > 1e15f || sample < -1e15f) sample = 0.0f;

    bool completed = false;

    // The audio path decimates 3.072 MHz to 24 kHz and halves that in software to 12 kHz,
    // so one 2048-sample DMA transfer (666.7 us on the 200 MHz M4) delivers only ~8 audio
    // samples. Two banks: 8 samples x 2 x 137 bins x ~12 instructions is ~26,300
    // instructions, some 132 us or a fifth of the chunk, against FT8's 2944 bin updates
    // in the same window. The 8192-sample symbol stretches each bank pass over 1024
    // chunks, and only one of them also carries the row-closing quantization (twice, once
    // per bank, in different chunks half a symbol apart).
    //
    // Bank 0 accumulates every sample and closes a row at stream sample 0 (mod 8192).
    goertzel_pass(sample, goertzel_s1[0], goertzel_s2[0]);
    state->bank0_samples_in_row++;
    if (state->bank0_samples_in_row >= WSPR_SAMPLES_PER_SYMBOL) {
        completed = true;
        finish_row(state, 0);
    }

    // Bank 1 runs the same stream on a row grid shifted half a symbol: it starts
    // accumulating at stream sample 4096 and closes its rows at 4096 (mod 8192). A
    // transmission at an arbitrary sub-row phase is therefore within a quarter symbol of
    // one bank's grid, which is what keeps the row-quantized decoder on its symbols.
    // bank1_started latches ONCE per session; total_samples may wrap (~99.4 h at 12 kHz)
    // and must not re-arm the half-symbol offset from scratch.
    if (!state->bank1_started) {
        if (state->total_samples >= WSPR_RX_BANK1_OFFSET) {
            state->bank1_started = true;
        }
    }
    if (state->bank1_started) {
        goertzel_pass(sample, goertzel_s1[1], goertzel_s2[1]);
        state->bank1_samples_in_row++;
        if (state->bank1_samples_in_row >= WSPR_SAMPLES_PER_SYMBOL) {
            completed = true;
            finish_row(state, 1);
        }
    }
    state->total_samples++;

    // True when this sample completed a block-row (published: total_br moved past it).
    // Bank 0 and bank 1 never close in the same sample — their boundaries sit half a
    // symbol apart — so at most one row completed here.
    return completed;
}

uint32_t wspr_portapack_total_br(const wspr_portapack_state_t* state) {
    if (!state) return 0;
    // Volatile load + compiler barrier: the reader's half of the publication protocol
    // (see wspr_decode.h). Single-core ordering only - the reader that observes this
    // value then sees every byte of the rows it covers, because the M4 is one core and
    // context switches preserve program order. This is NOT a C11 data-race-free claim.
    const uint32_t br = state->total_br;
    WSPR_RING_COMPILER_BARRIER();
    return br;
}

uint32_t wspr_portapack_valid_rows(const wspr_portapack_state_t* state) {
    if (!state) return 0;
    const uint32_t br = state->total_br;
    return br < (uint32_t)WSPR_RX_BLOCK_ROWS ? br : (uint32_t)WSPR_RX_BLOCK_ROWS;
}

const uint8_t* wspr_portapack_matrix(void) {
    return magnitude;
}
