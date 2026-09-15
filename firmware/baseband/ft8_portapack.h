/*
 * FT8 Portapack Adapter
 * Bridges ft8_lib with Portapack Mayhem baseband infrastructure
 *
 * Key adaptations:
 * - Uses Goertzel algorithm for exact 6.25 Hz bin spacing (matches FT8 tone spacing)
 * - Incremental sample processing (no burst computation)
 * - Optimized for Portapack's 96KB RAM (M4 core)
 * - Integration with existing Portapack DSP chain
 */

#ifndef _FT8_PORTAPACK_H_
#define _FT8_PORTAPACK_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "ft8_lib/decode.h"
#include "ft8_lib/message.h"

// FT8 timing constants
#define FT8_SLOT_DURATION      12.64f    // Length of a transmission; the T/R slot is 15 s
#define FT8_NUM_SYMBOLS        79        // Total symbols in FT8 message
#define FT8_SYNC_SYMBOLS       7         // Costas sync symbols
#define FT8_TONE_SPACING       6.25f     // Hz between tones
#define FT8_SYMBOL_RATE        6.25f     // Symbols per second

// Audio processing parameters
// Input is 24kHz, software decimation 2:1 gives 12kHz for Goertzel
#define FT8_SAMPLE_RATE        12000     // Goertzel sample rate
#define FT8_SAMPLES_PER_SYMBOL 1920      // 160ms * 12kHz = 1920 samples per symbol
// DFT size = samples per symbol = 1920
// This gives bin spacing = 12000/1920 = 6.25 Hz — EXACTLY matches FT8 tone spacing
#define FT8_FFT_SIZE           FT8_SAMPLES_PER_SYMBOL

// Frequency range (audio frequencies after USB demodulation)
// FT8 signals occupy 200-2500 Hz in USB audio
#define FT8_FREQ_MIN           200.0f    // Minimum frequency (Hz)
#define FT8_FREQ_MAX           2500.0f   // Maximum frequency (Hz)
#define FT8_FREQ_MIN_BIN       32        // 200 Hz / 6.25 Hz = bin 32
#define FT8_NUM_BINS           368       // Frequency bins covering 200-2500 Hz (2300/6.25=368)

// Memory optimization
#define FT8_MAX_CANDIDATES     15        // Maximum decode candidates
#define FT8_MAX_MESSAGES       10        // Maximum messages per slot
#define FT8_LDPC_ITERATIONS    10        // LDPC iterations
// Lowest Costas score a candidate is worth trying to decode. Anything that gets past the
// LDPC decoder still has to match a 14-bit CRC, so weak candidates cost a decode attempt
// rather than a false message, and there is nothing to gain by raising this.
#define FT8_MIN_SYNC_SCORE     10

// Waterfall buffer configuration
// A transmission is 79 symbols (12.64 s) but the T/R slot is 15 s, so the analysis
// window is made longer than the transmission. The extra 12 blocks are the slack the
// slot-tracking loop needs: the transmission is parked in the middle and clock error
// moves it within that slack instead of off the end of the buffer.
#define FT8_WATERFALL_BLOCKS   91        // 14.56 s; the remaining 440 ms runs the decoder
#define FT8_WATERFALL_TIME_OSR 1         // No time oversampling
#define FT8_WATERFALL_FREQ_OSR 1         // No frequency oversampling

// Waterfall size: 91 * 1 * 1 * 368 = 33,488 bytes (uint8_t)
#define FT8_WATERFALL_SIZE (FT8_WATERFALL_BLOCKS * FT8_WATERFALL_TIME_OSR * \
                            FT8_WATERFALL_FREQ_OSR * FT8_NUM_BINS)

// FT8 decoder state
typedef struct {
    // Waterfall data
    ftx_waterfall_t waterfall;
    WF_ELEM_T* waterfall_data;

    // Decode candidates
    ftx_candidate_t candidates[FT8_MAX_CANDIDATES];
    int num_candidates;

    // Decoded messages
    ftx_message_t messages[FT8_MAX_MESSAGES];
    int16_t message_scores[FT8_MAX_MESSAGES];  // Sync score per decoded message
    int num_messages;

    // Statistics
    uint32_t slot_count;
    uint32_t decode_count;
    uint32_t error_count;
    uint32_t skip_count;

    // Slot synchronisation observables, refreshed by ft8_portapack_decode().
    // Every station on the band keys to the same slot boundary, so the strongest
    // candidate's time_offset is a direct measurement of our own misalignment.
    int16_t sync_time_offset;  // Block where the transmission starts
    int16_t sync_score;        // Costas score of that candidate; 0 when none was found
    // True when sync_time_offset came from a candidate that passed CRC. The Costas array
    // repeats every FT8_SYNC_OFFSET = 36 blocks, so a window misaligned by one full
    // period still matches two of the three sync groups and scores in the thirties while
    // every data symbol is 36 places out. Score alone cannot tell the two apart, and
    // neither can repeating the measurement, since the alias is just as stable. A decode
    // can: it never succeeds on the aliased alignment.
    bool sync_confirmed;


    // Status flags
    bool initialized;
    bool decoding_active;

    // Goertzel incremental state
    int samples_in_symbol;  // Count of samples processed in current symbol

    // Debug (minimal - keep only useful fields)
    float debug_audio_peak;
    int debug_blocks_written;
    int max_magnitude;

} ft8_decoder_state_t;

#ifdef __cplusplus
extern "C" {
#endif

// Initialize FT8 decoder (precomputes Goertzel coefficients)
bool ft8_portapack_init(ft8_decoder_state_t* state);

// Free FT8 decoder resources
void ft8_portapack_free(ft8_decoder_state_t* state);

// Feed one audio sample at 12kHz to the Goertzel filters (incremental processing)
// Returns true when the analysis window (FT8_WATERFALL_BLOCKS symbols) is full
bool ft8_portapack_feed_sample(ft8_decoder_state_t* state, float sample);

// Decode current waterfall data
// Returns number of successfully decoded messages
int ft8_portapack_decode(ft8_decoder_state_t* state);

// Reset decoder for new slot
void ft8_portapack_reset_slot(ft8_decoder_state_t* state);


#ifdef __cplusplus
}
#endif

#endif // _FT8_PORTAPACK_H_
