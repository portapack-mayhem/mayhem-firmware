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

#ifndef _FT8_PORTAPACK_H_
#define _FT8_PORTAPACK_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "ft8_lib/decode.h"
#include "ft8_lib/message.h"

#define FT8_NUM_SYMBOLS 79

// ft8_lib's decoder assumes one waterfall bin equals one FT8 tone, so the bin spacing has
// to be exactly 6.25 Hz. At 12 kHz that means a 1920-point transform; a 2048-point FFT
// gives 5.86 Hz and breaks sync detection.
#define FT8_SAMPLE_RATE 12000
#define FT8_SAMPLES_PER_SYMBOL 1920

// FT8 occupies 200-2500 Hz of the USB audio, which is DFT bins 32..399.
#define FT8_FREQ_MIN_BIN 32
#define FT8_NUM_BINS 368

#define FT8_MAX_CANDIDATES 15
#define FT8_MAX_MESSAGES 10
#define FT8_LDPC_ITERATIONS 10

// Anything past the LDPC decoder still has to match a 14-bit CRC, so a weak candidate
// costs a decode attempt rather than a false message.
#define FT8_MIN_SYNC_SCORE 10

// A transmission is 79 symbols but the T/R slot is 15 s, so the window is longer than the
// transmission. The extra 12 blocks are the slack the slot-tracking loop moves within.
#define FT8_WATERFALL_BLOCKS 91
#define FT8_WATERFALL_TIME_OSR 1
#define FT8_WATERFALL_FREQ_OSR 1

#define FT8_WATERFALL_SIZE (FT8_WATERFALL_BLOCKS * FT8_WATERFALL_TIME_OSR * \
                            FT8_WATERFALL_FREQ_OSR * FT8_NUM_BINS)

typedef struct {
    ftx_waterfall_t waterfall;

    ftx_candidate_t candidates[FT8_MAX_CANDIDATES];
    int num_candidates;

    ftx_message_t messages[FT8_MAX_MESSAGES];
    // Audio frequency each message was decoded at, in Hz. Stations pick their own slot
    // inside the 200-2500 Hz passband, so this is what tells them apart on one dial.
    int16_t message_freqs[FT8_MAX_MESSAGES];
    int num_messages;

    // Every station on the band keys to the same slot boundary, so the time_offset of the
    // best candidate measures our own misalignment.
    int16_t sync_time_offset;
    int16_t sync_score;
    // Set when sync_time_offset came from a candidate that passed CRC. The Costas array
    // repeats every 36 blocks, so a window misaligned by one full period still matches two
    // of the three sync groups and scores in the thirties while every data symbol is 36
    // places out. Score cannot tell the two apart and the alias is just as stable across
    // slots, but a decode never succeeds on it.
    bool sync_confirmed;

    bool initialized;
    int samples_in_symbol;
} ft8_decoder_state_t;

#ifdef __cplusplus
extern "C" {
#endif

bool ft8_portapack_init(ft8_decoder_state_t* state);
void ft8_portapack_free(ft8_decoder_state_t* state);

// Returns true once the analysis window holds FT8_WATERFALL_BLOCKS symbols.
bool ft8_portapack_feed_sample(ft8_decoder_state_t* state, float sample);

// Returns the number of messages decoded from the captured window.
int ft8_portapack_decode(ft8_decoder_state_t* state);

void ft8_portapack_reset_slot(ft8_decoder_state_t* state);

// text must hold FTX_MAX_MESSAGE_LENGTH bytes. It comes back empty for a payload of a
// type the library does not unpack.
void ft8_portapack_message_text(const ftx_message_t* message, char* text);

#ifdef __cplusplus
}
#endif

#endif  // _FT8_PORTAPACK_H_
