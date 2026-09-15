/*
 * FT8 Portapack Implementation
 * Core integration layer between ft8_lib and Portapack Mayhem
 *
 * Uses Goertzel algorithm instead of FFT for exact 6.25 Hz bin spacing.
 * This is critical: ft8_lib's decoder assumes 1 waterfall bin = 1 FT8 tone (6.25 Hz).
 * A 2048-point FFT at 12kHz gives 5.86 Hz/bin which breaks sync detection.
 * Goertzel with N=1920 at 12kHz gives exactly 12000/1920 = 6.25 Hz/bin.
 */

#include "ft8_portapack.h"
#include "ft8_lib/decode.h"
#include "ft8_lib/constants.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

// --- Static buffers (BSS section) ---

// Waterfall magnitude data: FT8_WATERFALL_BLOCKS * FT8_NUM_BINS bytes
static WF_ELEM_T waterfall_buffer[FT8_WATERFALL_SIZE];

// Goertzel precomputed coefficients: 2*cos(2*pi*k/N) for each bin
// 368 bins * 4 bytes = 1,472 bytes
static float goertzel_coeff[FT8_NUM_BINS];

// Goertzel filter state (persistent across samples within one symbol)
// 368 bins * 4 bytes * 2 = 2,944 bytes
static float goertzel_s1[FT8_NUM_BINS];
static float goertzel_s2[FT8_NUM_BINS];

// Total static RAM: 33,488 + 1,472 + 2,944 = 37,904 bytes, inside the M4's 96 KB.

// --- Internal helpers ---

// Cosine over [0, pi/2], which is the whole range the coefficient table needs:
// the highest DFT bin is FT8_FREQ_MIN_BIN + FT8_NUM_BINS - 1 = 399, giving
// 2*pi*399/1920 = 1.306 rad. Calling cosf() instead drags in newlib's argument
// reduction, __kernel_rem_pio2f plus __ieee754_rem_pio2f, which costs 2,304 bytes of
// the 32 KB external-app budget for a routine used once at startup. This Taylor series
// stays within 4.8e-10 of cos() across all 368 bins, far below what the Goertzel
// coefficient needs.
static float cos_unit_quadrant(float x) {
    const float x2 = x * x;
    return 1.0f + x2 * (-1.0f / 2.0f + x2 * (1.0f / 24.0f + x2 * (-1.0f / 720.0f + x2 * (1.0f / 40320.0f + x2 * (-1.0f / 3628800.0f + x2 * (1.0f / 479001600.0f))))));
}

// Finish one symbol: compute Goertzel power, convert to absolute dB, quantize to the
// uint8_t range the decoder expects (decode.h WF_ELEM_MAG: mag_dB = byte * 0.5 - 120,
// so byte = clamp((power_dB + 120) * 2, 0, 255)).
//
// The scale factor matters. The Goertzel sum for an on-bin tone of amplitude A is
// A*N/2, so dividing the power by N*N puts a full-scale tone at A^2/4 — the same
// reference upstream ft8_lib uses (common/monitor.c applies fft_norm = 2/nfft to its
// window). Dividing by N instead, as this code did, runs the scale 32.8 dB hot: a tone
// above roughly 0.1 of full scale pins at 255 together with the skirt in its neighbour
// bins, and since ft8_sync_score is nothing but tone-minus-neighbour differences, the
// score collapses. Measured on the host testbench: with /N the decode rate falls from
// 85% to 3% as the tone grows from 0.2 to 0.7 of full scale; with /(N*N) it stays at
// 80-85% across that whole range. There is no AGC in front of this, so the input level
// is whatever the user's LNA and VGA produce.
static void finish_goertzel_symbol(ft8_decoder_state_t* state) {
    int block_offset = state->waterfall.num_blocks * state->waterfall.block_stride;

    for (int b = 0; b < FT8_NUM_BINS; b++) {
        float s1 = goertzel_s1[b];
        float s2 = goertzel_s2[b];
        // Goertzel power: |X[k]|^2 = s1^2 + s2^2 - coeff*s1*s2
        float power = s1 * s1 + s2 * s2 - goertzel_coeff[b] * s1 * s2;
        power *= (1.0f / ((float)FT8_SAMPLES_PER_SYMBOL * (float)FT8_SAMPLES_PER_SYMBOL));
        if (power < 1e-20f) power = 1e-20f;  // Avoid log10(0) / denormals.

        // Absolute power in dB, quantized to match the decoder's native 0.5 dB step.
        float power_db = 10.0f * log10f(power);
        int m = (int)((power_db + 120.0f) * 2.0f);
        if (m < 0) m = 0;
        if (m > 255) m = 255;
        state->waterfall.mag[block_offset + b] = (WF_ELEM_T)m;
    }

    state->debug_blocks_written = state->waterfall.num_blocks + 1;

    // Reset Goertzel state for next symbol
    memset(goertzel_s1, 0, sizeof(goertzel_s1));
    memset(goertzel_s2, 0, sizeof(goertzel_s2));
    state->samples_in_symbol = 0;

    state->waterfall.num_blocks++;
}

// --- Public API ---

bool ft8_portapack_init(ft8_decoder_state_t* state) {
    if (!state) return false;

    memset(state, 0, sizeof(ft8_decoder_state_t));

    // Setup waterfall structure
    state->waterfall.max_blocks = FT8_WATERFALL_BLOCKS;
    state->waterfall.num_blocks = 0;
    state->waterfall.num_bins = FT8_NUM_BINS;
    state->waterfall.time_osr = FT8_WATERFALL_TIME_OSR;
    state->waterfall.freq_osr = FT8_WATERFALL_FREQ_OSR;
    state->waterfall.mag = waterfall_buffer;
    state->waterfall.block_stride = FT8_WATERFALL_TIME_OSR * FT8_WATERFALL_FREQ_OSR * FT8_NUM_BINS;
    state->waterfall.protocol = FTX_PROTOCOL_FT8;

    state->waterfall_data = waterfall_buffer;
    state->initialized = true;
    state->decoding_active = false;

    state->skip_count = 0;
    state->samples_in_symbol = 0;

    // Precompute Goertzel coefficients for bins [FT8_FREQ_MIN_BIN .. FT8_FREQ_MIN_BIN + FT8_NUM_BINS - 1]
    // Each bin k corresponds to frequency k * 6.25 Hz
    // Goertzel coefficient = 2 * cos(2 * pi * k / N) where N = 1920
    const float two_pi_over_n = 2.0f * 3.14159265358979f / (float)FT8_SAMPLES_PER_SYMBOL;
    for (int b = 0; b < FT8_NUM_BINS; b++) {
        int k = FT8_FREQ_MIN_BIN + b;  // DFT bin index (32..399)
        goertzel_coeff[b] = 2.0f * cos_unit_quadrant(two_pi_over_n * k);
    }

    // Clear Goertzel state
    memset(goertzel_s1, 0, sizeof(goertzel_s1));
    memset(goertzel_s2, 0, sizeof(goertzel_s2));

    return true;
}

void ft8_portapack_free(ft8_decoder_state_t* state) {
    if (!state) return;
    state->initialized = false;
    state->decoding_active = false;
}

bool ft8_portapack_feed_sample(ft8_decoder_state_t* state, float sample) {
    if (!state || !state->initialized) return false;

    // Slot already complete — signal caller to decode
    if (state->waterfall.num_blocks >= FT8_WATERFALL_BLOCKS) {
        return true;
    }

    // NaN/Inf protection — Inf passes NaN check (Inf==Inf is true)
    if (sample != sample || sample > 1e15f || sample < -1e15f) sample = 0.0f;

    // Track peak for debug
    float abs_s = (sample < 0.0f) ? -sample : sample;
    if (abs_s > state->debug_audio_peak) state->debug_audio_peak = abs_s;

    // Update all Goertzel filters with this sample.
    // Measured from the -Os disassembly this is about 12 instructions per bin, so
    // 368 bins at 12 kHz costs roughly 36-48% of the 204 MHz M4. That is the reason
    // time_osr stays at 1: a second bank would not fit the budget. Moving to a
    // 1920-point mixed-radix FFT would cut this by two orders of magnitude and make
    // oversampling affordable.
    for (int b = 0; b < FT8_NUM_BINS; b++) {
        float s0 = sample + goertzel_coeff[b] * goertzel_s1[b] - goertzel_s2[b];
        goertzel_s2[b] = goertzel_s1[b];
        goertzel_s1[b] = s0;
    }

    state->samples_in_symbol++;

    // Check if symbol is complete (1920 samples = 160ms at 12kHz)
    if (state->samples_in_symbol >= FT8_SAMPLES_PER_SYMBOL) {
        finish_goertzel_symbol(state);
        return (state->waterfall.num_blocks >= FT8_WATERFALL_BLOCKS);
    }

    return false;
}

int ft8_portapack_decode(ft8_decoder_state_t* state) {
    if (!state || !state->initialized) return 0;
    if (state->waterfall.num_blocks < FT8_WATERFALL_BLOCKS) return 0;

    state->decoding_active = true;
    state->num_messages = 0;

    // Calculate max magnitude for debug
    int max_mag = 0;
    for (int block = 0; block < state->waterfall.num_blocks; block++) {
        int base = block * state->waterfall.block_stride;
        for (int bin = 1; bin < state->waterfall.block_stride; bin++) {
            WF_ELEM_T mag = state->waterfall.mag[base + bin];
            if (mag > max_mag) max_mag = mag;
        }
    }
    state->max_magnitude = max_mag;

    // The candidate search doubles as the slot-timing sensor, so its threshold is fixed:
    // a search that can be tuned past the strongest signal's score would blind the sync
    // loop and leave the receiver drifting with no way back.
    state->num_candidates = ftx_find_candidates(&state->waterfall,
                                                FT8_MAX_CANDIDATES,
                                                state->candidates,
                                                FT8_MIN_SYNC_SCORE);

    if (state->num_candidates >= FT8_MAX_CANDIDATES) {
        state->skip_count++;
    }

    // Provisional slot-sync measurement from the strongest Costas match. Every station
    // keys to the same T/R boundary, so this number is the caller's alignment error —
    // unless it is the 36-block alias, which the decode loop below rules out.
    state->sync_confirmed = false;
    if (state->num_candidates > 0) {
        state->sync_time_offset = state->candidates[0].time_offset;
        state->sync_score = state->candidates[0].score;
    } else {
        state->sync_time_offset = 0;
        state->sync_score = 0;
    }

    // Decode candidates (with duplicate filtering)
    for (int i = 0; i < state->num_candidates && state->num_messages < FT8_MAX_MESSAGES; i++) {
        ftx_message_t msg;
        ftx_decode_status_t st;
        if (ftx_decode_candidate(&state->waterfall, &state->candidates[i],
                                 FT8_LDPC_ITERATIONS, &msg, &st) &&
            st.ldpc_errors == 0) {
            // Check for duplicate payload (same signal decoded at different time/freq offsets)
            bool duplicate = false;
            for (int j = 0; j < state->num_messages; j++) {
                if (memcmp(msg.payload, state->messages[j].payload, FTX_PAYLOAD_LENGTH_BYTES) == 0) {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate) continue;

            // A CRC-valid decode proves this candidate is the real alignment, not the
            // Costas alias, so it overrides the provisional measurement above.
            if (!state->sync_confirmed) {
                state->sync_time_offset = state->candidates[i].time_offset;
                state->sync_score = state->candidates[i].score;
                state->sync_confirmed = true;
            }
            state->messages[state->num_messages] = msg;
            state->message_scores[state->num_messages] = state->candidates[i].score;
            state->num_messages++;
            state->decode_count++;
        }
    }

    state->decoding_active = false;
    state->slot_count++;

    return state->num_messages;
}

void ft8_portapack_message_text(const ftx_message_t* message, char* text) {
    ftx_message_offsets_t offsets;

    // A hashed callsign that no table can resolve renders as <...> and is reported as an
    // error, so the return code is ignored and the text kept: the rest of the message is
    // still worth showing. Holding a hash table would mean keeping every callsign heard
    // this session. Payloads of a type the library does not unpack come back empty,
    // because ftx_message_decode() terminates the string before it dispatches.
    ftx_message_decode(message, NULL, text, &offsets);
}

void ft8_portapack_reset_slot(ft8_decoder_state_t* state) {
    if (!state) return;
    // Clear the buffer before publishing num_blocks = 0, so the baseband thread can
    // never observe an empty block count while stale magnitudes are still in place.
    memset(state->waterfall.mag, 0, FT8_WATERFALL_SIZE);
    state->num_candidates = 0;
    state->num_messages = 0;
    state->debug_audio_peak = 0.0f;
    // Start the next window on a clean symbol boundary: the capture phase is shifted
    // between slots, so any partial symbol left here would corrupt block 0.
    state->samples_in_symbol = 0;
    memset(goertzel_s1, 0, sizeof(goertzel_s1));
    memset(goertzel_s2, 0, sizeof(goertzel_s2));
    state->waterfall.num_blocks = 0;
}
