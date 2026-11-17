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

#include <string.h>

// Column b of the matrix is absolute bin WSPR_RX_FIRST_BIN + b of the 8192-point
// transform; bin 956 is the integer bin nearest the 1400 Hz band edge
// (956 * 12000/8192 = 1400.39 Hz).
#define WSPR_RX_FIRST_BIN 956

// The matrix and the banks are file-static in .bss, like the FT8 waterfall: 48224 matrix
// bytes (352 block-rows, two interleaved banks) plus 3 x 600 bytes of shared coefficient
// bank and 2 x (2 x 600) bytes of per-bank state, 55.4 KB against the FT8 glue's 37 KB.
// Keeping them out of the state struct leaves the state a handful of counters.
static uint8_t magnitude[WSPR_RX_MATRIX_SIZE];
static float goertzel_coeff[WSPR_RX_NUM_BINS];
static float goertzel_s1[WSPR_RX_TIME_OSR][WSPR_RX_NUM_BINS];
static float goertzel_s2[WSPR_RX_TIME_OSR][WSPR_RX_NUM_BINS];

// Cosine over [0, pi/2], the whole range the coefficient table needs: the highest bin is
// 1105, giving 2*pi*1105/8192 = 0.85 rad. cosf() would drag in newlib's argument
// reduction, __kernel_rem_pio2f plus __ieee754_rem_pio2f, at 2,304 bytes of the 32 KB app
// budget for a routine used once at startup. Error stays within 1.2e-12 across all 150
// bins.
static float cos_unit_quadrant(float x) {
    const float x2 = x * x;
    return 1.0f + x2 * (-1.0f / 2.0f + x2 * (1.0f / 24.0f + x2 * (-1.0f / 720.0f + x2 * (1.0f / 40320.0f + x2 * (-1.0f / 3628800.0f + x2 * (1.0f / 479001600.0f))))));
}

// log2 from the exponent plus a series on the mantissa. Calling log10f here costs 146
// instructions per bin, and the row-closing pass runs all 150 of them inside the one DMA
// chunk that closes a symbol — a chunk that also carries that chunk's own 8 per-sample
// passes. log10f would push that worst-case chunk from about 100 us past 200 us of the
// 666.7 us budget, and drag newlib's log10 tables into the app besides. The decoder only
// ever weighs bytes against their neighbours, so any monotonic affine map of dB will do.
static float log2_from_bits(float x) {
    union {
        float f;
        uint32_t i;
    } u;
    u.f = x;
    int e = (int)((u.i >> 23) & 0xFFu) - 127;
    u.i = (u.i & 0x007FFFFFu) | 0x3F800000u;
    float m = u.f;
    // Centring the mantissa on 1 keeps |t| <= 0.1716, where the first dropped term of the
    // series is 7e-7, against a quantisation step of 0.166 in log2 units.
    if (m > 1.4142136f) {
        m *= 0.5f;
        e++;
    }
    const float t = (m - 1.0f) / (m + 1.0f);
    const float t2 = t * t;
    return (float)e + t * (2.8853900f + t2 * (0.9617967f + t2 * 0.5770780f));
}

// Quantize one row's Goertzel power to the byte scale the decoder reads
// (byte = 20*log10(power/N^2) + 240, the scale the FT8 waterfall uses and the wspr
// decoder tests are calibrated to: a byte step is 0.5 dB).
//
// The Goertzel sum for an on-bin tone of amplitude A is A*N/2, so dividing the power by
// N*N puts a full-scale tone at A^2/4, byte 228. Dividing by N instead runs the scale
// 39.1 dB hot at N = 8192: a tone above roughly 0.02 of full scale pins at 255 together
// with its neighbour bins, and the decoder only ever compares bytes against their
// neighbours, so the scale would collapse. There is no AGC ahead of this stage.
static void finish_row(wspr_portapack_state_t* state, int bank) {
    // Block-row = 2*symbol_row + bank, the interleaved layout wspr_decode_window() walks
    // with time_osr = 2.
    const int rows = bank ? state->bank1_rows_filled : state->rows_filled;
    const int row_offset = (WSPR_RX_TIME_OSR * rows + bank) * WSPR_RX_NUM_BINS;

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
        magnitude[row_offset + b] = (uint8_t)m;
    }

    memset(goertzel_s1[bank], 0, sizeof(goertzel_s1[bank]));
    memset(goertzel_s2[bank], 0, sizeof(goertzel_s2[bank]));
    if (bank) {
        state->bank1_samples_in_row = 0;
        state->bank1_rows_filled++;
    } else {
        state->samples_in_row = 0;
        state->rows_filled++;
        if (state->rows_filled >= WSPR_RX_NUM_ROWS) {
            // Bank 1 is still half a symbol short of its own last row boundary here; its
            // row 175 never closes and its bytes stay zero, which the decoder treats as
            // an erased symbol for the (at most) t = 14 candidate of that bank.
            state->window_full = true;
        }
    }
}

bool wspr_portapack_init(wspr_portapack_state_t* state) {
    if (!state) return false;

    memset(state, 0, sizeof(wspr_portapack_state_t));

    // The table, the banks and the matrix are file-static, so a restart of the app has to
    // clear them.
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
    // so a stale pointer cannot keep reporting a full window.
    memset(state, 0, sizeof(wspr_portapack_state_t));
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

    if (state->window_full) {
        return true;
    }

    // Inf passes the NaN check, since Inf == Inf holds.
    if (sample != sample || sample > 1e15f || sample < -1e15f) sample = 0.0f;

    // The audio path decimates 3.072 MHz to 24 kHz and halves that in software to 12 kHz,
    // so one 2048-sample DMA transfer (666.7 us on the 200 MHz M4) delivers only ~8 audio
    // samples. Two banks now: 8 samples x 2 x 137 bins x ~12 instructions is ~26,300
    // instructions, some 132 us or a fifth of the chunk, against FT8's 2944 bin updates
    // in the same window. The 8192-sample symbol stretches each bank pass over 1024
    // chunks, and only one of them also carries the row-closing quantization (twice, once
    // per bank, in different chunks half a symbol apart).
    //
    // Bank 0 accumulates every sample and closes a row at stream sample 0 (mod 8192).
    goertzel_pass(sample, goertzel_s1[0], goertzel_s2[0]);
    state->samples_in_row++;
    if (state->samples_in_row >= WSPR_SAMPLES_PER_SYMBOL) {
        finish_row(state, 0);
    }

#if WSPR_RX_TIME_OSR == 2
    // Bank 1 runs the same stream on a row grid shifted half a symbol: it starts
    // accumulating at stream sample 4096 and closes its rows at 4096 (mod 8192). A
    // transmission at an arbitrary sub-row phase is therefore within a quarter symbol of
    // one bank's grid, which is what keeps the row-quantized decoder on its symbols.
    if (state->total_samples >= WSPR_RX_BANK1_OFFSET) {
        goertzel_pass(sample, goertzel_s1[1], goertzel_s2[1]);
        state->bank1_samples_in_row++;
        if (state->bank1_samples_in_row >= WSPR_SAMPLES_PER_SYMBOL) {
            finish_row(state, 1);
        }
    }
#endif
    state->total_samples++;

    return state->window_full;
}

const uint8_t* wspr_portapack_matrix(void) {
    return magnitude;
}

int wspr_portapack_rows(void) {
    return WSPR_RX_NUM_ROWS;
}

int wspr_portapack_time_osr(void) {
    return WSPR_RX_TIME_OSR;
}

int wspr_portapack_bins(void) {
    return WSPR_RX_NUM_BINS;
}

int wspr_portapack_bin0_hz(void) {
    return WSPR_RX_BIN0_HZ;
}

void wspr_portapack_reset_window(wspr_portapack_state_t* state) {
    if (!state) return;
    // Clear the buffer before publishing the empty counters, so the decode step can never
    // observe rows_filled = 0 while stale magnitudes are still in place.
    memset(magnitude, 0, sizeof(magnitude));
    // A partial row left in the banks would corrupt row 0 of the next window: the capture
    // phase shifts between slots, so the banks restart from zero before the counters do.
    memset(goertzel_s1, 0, sizeof(goertzel_s1));
    memset(goertzel_s2, 0, sizeof(goertzel_s2));
    state->rows_filled = 0;
    state->samples_in_row = 0;
    state->window_full = false;
    // Bank 1 keeps its own row counters (finish_row derives its matrix row from
    // bank1_rows_filled); missing these here let the derived offset grow past the
    // matrix by ~137 bytes per window, writing into the heap arena in window 3 and
    // past the end of RAM in window 4.
    state->bank1_rows_filled = 0;
    state->bank1_samples_in_row = 0;
    state->total_samples = 0;
}
