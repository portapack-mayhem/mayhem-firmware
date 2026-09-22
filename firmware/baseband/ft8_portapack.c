/*
 * Copyright (C) 2026 Dmytro Onyshko
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

#include "ft8_portapack.h"
#include "ft8_lib/decode.h"
#include "ft8_lib/constants.h"

#include <stdlib.h>
#include <string.h>

static WF_ELEM_T waterfall_buffer[FT8_WATERFALL_SIZE];
static float goertzel_coeff[FT8_NUM_BINS];
static float goertzel_s1[FT8_NUM_BINS];
static float goertzel_s2[FT8_NUM_BINS];

// Cosine over [0, pi/2], the whole range the coefficient table needs: the highest bin is
// 399, giving 2*pi*399/1920 = 1.306 rad. cosf() would drag in newlib's argument reduction,
// __kernel_rem_pio2f plus __ieee754_rem_pio2f, at 2,304 bytes of the 32 KB app budget for
// a routine used once at startup. Error stays within 4.8e-10 across all 368 bins.
static float cos_unit_quadrant(float x) {
    const float x2 = x * x;
    return 1.0f + x2 * (-1.0f / 2.0f + x2 * (1.0f / 24.0f + x2 * (-1.0f / 720.0f + x2 * (1.0f / 40320.0f + x2 * (-1.0f / 3628800.0f + x2 * (1.0f / 479001600.0f))))));
}

// log2 from the exponent plus a series on the mantissa. Calling log10f here costs 146
// instructions per bin, and 368 of them in the one execute() that closes a symbol overrun
// the 666.7 us a 2048-sample DMA transfer allows, so the baseband thread lost one buffer
// per symbol. decode.h quantises to 0.5 dB steps and ft8_sync_score only ever takes
// differences between neighbouring bytes, so any monotonic affine map of dB will do.
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

// Quantize one symbol's Goertzel power to the byte scale decode.h expects
// (WF_ELEM_MAG: mag_dB = byte * 0.5 - 120, so byte = 20*log10(power) + 240).
//
// The Goertzel sum for an on-bin tone of amplitude A is A*N/2, so dividing the power by
// N*N puts a full-scale tone at A^2/4, the reference upstream ft8_lib uses. Dividing by N
// instead runs the scale 32.8 dB hot: a tone above roughly 0.1 of full scale pins at 255
// together with its neighbour bins, and ft8_sync_score is nothing but tone-minus-neighbour
// differences, so the score collapses. There is no AGC ahead of this stage.
static void finish_goertzel_symbol(ft8_decoder_state_t* state) {
    int block_offset = state->waterfall.num_blocks * state->waterfall.block_stride;

    for (int b = 0; b < FT8_NUM_BINS; b++) {
        float s1 = goertzel_s1[b];
        float s2 = goertzel_s2[b];
        float power = s1 * s1 + s2 * s2 - goertzel_coeff[b] * s1 * s2;
        power *= (1.0f / ((float)FT8_SAMPLES_PER_SYMBOL * (float)FT8_SAMPLES_PER_SYMBOL));
        if (power < 1e-20f) power = 1e-20f;

        // 20/log2(10) = 6.0206 converts the log2 above into the byte scale.
        int m = (int)(6.0206f * log2_from_bits(power) + 240.0f);
        if (m < 0) m = 0;
        if (m > 255) m = 255;
        state->waterfall.mag[block_offset + b] = (WF_ELEM_T)m;
    }

    memset(goertzel_s1, 0, sizeof(goertzel_s1));
    memset(goertzel_s2, 0, sizeof(goertzel_s2));
    state->samples_in_symbol = 0;

    state->waterfall.num_blocks++;
}

bool ft8_portapack_init(ft8_decoder_state_t* state) {
    if (!state) return false;

    memset(state, 0, sizeof(ft8_decoder_state_t));

    state->waterfall.max_blocks = FT8_WATERFALL_BLOCKS;
    state->waterfall.num_blocks = 0;
    state->waterfall.num_bins = FT8_NUM_BINS;
    state->waterfall.time_osr = FT8_WATERFALL_TIME_OSR;
    state->waterfall.freq_osr = FT8_WATERFALL_FREQ_OSR;
    state->waterfall.mag = waterfall_buffer;
    state->waterfall.block_stride = FT8_WATERFALL_TIME_OSR * FT8_WATERFALL_FREQ_OSR * FT8_NUM_BINS;
    state->waterfall.protocol = FTX_PROTOCOL_FT8;

    state->initialized = true;
    state->samples_in_symbol = 0;

    const float two_pi_over_n = 2.0f * 3.14159265358979f / (float)FT8_SAMPLES_PER_SYMBOL;
    for (int b = 0; b < FT8_NUM_BINS; b++) {
        int k = FT8_FREQ_MIN_BIN + b;
        goertzel_coeff[b] = 2.0f * cos_unit_quadrant(two_pi_over_n * k);
    }

    memset(goertzel_s1, 0, sizeof(goertzel_s1));
    memset(goertzel_s2, 0, sizeof(goertzel_s2));

    return true;
}

void ft8_portapack_free(ft8_decoder_state_t* state) {
    if (!state) return;
    state->initialized = false;
}

bool ft8_portapack_feed_sample(ft8_decoder_state_t* state, float sample) {
    if (!state || !state->initialized) return false;

    if (state->waterfall.num_blocks >= FT8_WATERFALL_BLOCKS) {
        return true;
    }

    // Inf passes the NaN check, since Inf == Inf holds.
    if (sample != sample || sample > 1e15f || sample < -1e15f) sample = 0.0f;

    // About 12 instructions per bin from the -Os disassembly, so 368 bins at 12 kHz take
    // roughly 280 us of the 666.7 us a DMA transfer allows on the 200 MHz M4. That is why
    // time_osr stays at 1: a second bank would not fit the budget.
    for (int b = 0; b < FT8_NUM_BINS; b++) {
        float s0 = sample + goertzel_coeff[b] * goertzel_s1[b] - goertzel_s2[b];
        goertzel_s2[b] = goertzel_s1[b];
        goertzel_s1[b] = s0;
    }

    state->samples_in_symbol++;

    if (state->samples_in_symbol >= FT8_SAMPLES_PER_SYMBOL) {
        finish_goertzel_symbol(state);
        return (state->waterfall.num_blocks >= FT8_WATERFALL_BLOCKS);
    }

    return false;
}

int ft8_portapack_decode(ft8_decoder_state_t* state) {
    if (!state || !state->initialized) return 0;
    if (state->waterfall.num_blocks < FT8_WATERFALL_BLOCKS) return 0;

    state->num_messages = 0;

    // The candidate search doubles as the slot-timing sensor, so its threshold is fixed: a
    // search that could be tuned past the strongest signal's score would blind the sync
    // loop and leave the receiver drifting with no way back.
    state->num_candidates = ftx_find_candidates(&state->waterfall,
                                                FT8_MAX_CANDIDATES,
                                                state->candidates,
                                                FT8_MIN_SYNC_SCORE);

    // Provisional measurement from the strongest Costas match, unless it is the 36-block
    // alias that the decode loop below rules out.
    state->sync_confirmed = false;
    if (state->num_candidates > 0) {
        state->sync_time_offset = state->candidates[0].time_offset;
        state->sync_score = state->candidates[0].score;
    } else {
        state->sync_time_offset = 0;
        state->sync_score = 0;
    }

    for (int i = 0; i < state->num_candidates && state->num_messages < FT8_MAX_MESSAGES; i++) {
        ftx_message_t msg;
        ftx_decode_status_t st;
        if (ftx_decode_candidate(&state->waterfall, &state->candidates[i],
                                 FT8_LDPC_ITERATIONS, &msg, &st) &&
            st.ldpc_errors == 0) {
            // One signal is often found at several time and frequency offsets.
            bool duplicate = false;
            for (int j = 0; j < state->num_messages; j++) {
                if (memcmp(msg.payload, state->messages[j].payload, FTX_PAYLOAD_LENGTH_BYTES) == 0) {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate) continue;

            // A CRC-valid decode proves this is the real alignment, not the alias.
            if (!state->sync_confirmed) {
                state->sync_time_offset = state->candidates[i].time_offset;
                state->sync_score = state->candidates[i].score;
                state->sync_confirmed = true;
            }
            state->messages[state->num_messages] = msg;
            state->message_scores[state->num_messages] = state->candidates[i].score;
            state->num_messages++;
        }
    }

    return state->num_messages;
}

void ft8_portapack_message_text(const ftx_message_t* message, char* text) {
    ftx_message_offsets_t offsets;

    // Standard and non-standard payloads keep their partially filled fields on an error
    // return, so a CRC-valid message whose callsign fails to unpack would otherwise print
    // a lone space or a truncated call. A hashed callsign renders as <...> and returns OK,
    // so nothing readable is lost here.
    if (ftx_message_decode(message, NULL, text, &offsets) != FTX_MESSAGE_RC_OK) {
        text[0] = '\0';
    }
}

void ft8_portapack_reset_slot(ft8_decoder_state_t* state) {
    if (!state) return;
    // Clear the buffer before publishing num_blocks = 0, so the baseband thread can never
    // observe an empty block count while stale magnitudes are still in place.
    memset(state->waterfall.mag, 0, FT8_WATERFALL_SIZE);
    state->num_candidates = 0;
    state->num_messages = 0;
    // The capture phase shifts between slots, so a partial symbol left here would corrupt
    // block 0 of the next window.
    state->samples_in_symbol = 0;
    memset(goertzel_s1, 0, sizeof(goertzel_s1));
    memset(goertzel_s2, 0, sizeof(goertzel_s2));
    state->waterfall.num_blocks = 0;
}
