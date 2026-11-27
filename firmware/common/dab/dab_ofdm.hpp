/*
 * Copyright (C) 2026 Mayhem firmware contributors
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

/* OFDM layer (M4): the 2048-point FFT, the phase reference symbol,
 * frequency interleaving and the demapper to soft bits. */

#ifndef __DAB_OFDM_H__
#define __DAB_OFDM_H__

#include "dab_common.hpp"

#include <cstddef>
#include <cstdint>

namespace dab {

/* ---- FFT ---- */

/* Fixed-point 2048-point FFT, 16-bit complex, Q15 twiddles (dab_ofdm.cpp).
 * It halves at stages 3, 5, 7 and 9 and saturates elsewhere, so the true
 * spectrum is output * 2^4. Halving every stage costs ~20 dB; halving later
 * saturates. */

struct FftComplex {
    int16_t re{0};
    int16_t im{0};
};

/* In place, natural order in and out. The unscaled spectrum is the output
 * times 2^out_shift. */
bool fft2048(FftComplex* data, uint8_t* out_shift = nullptr);

/* fft2048() in kFftSteps pieces, so the per-buffer hook can advance it without
 * monopolising the thread (a transform outlasts a symbol). Byte
 * for byte fft2048()'s result; leave the buffer alone until advance() is true. */
constexpr uint8_t kFftSteps = 7;

struct FftProgress {
    FftComplex* data{nullptr};
    uint8_t step{kFftSteps}; /* == kFftSteps means idle or finished */
    uint8_t shift{0};        /* set when the last step completes */
};

void fft2048_begin(FftProgress& progress, FftComplex* data);

/* At most `budget` steps; true when complete. */
bool fft2048_advance(FftProgress& progress, uint8_t budget);

/* Exposed so a caller that copies its input can fold the reversal into it. */
size_t fft_bit_reverse_index(size_t index);

/* ---- Phase reference symbol ---- */

/* Mode I phase reference symbol (EN 300 401 section 14.3), the only absolute
 * timing and frequency reference: correlated against it, the spectrum gives the
 * channel impulse response and the integer carrier offset. Phases are packed
 * two bits per carrier (384 bytes) at compile time from tables 43 and 44. */

/* The PRS is defined on carrier index k and is NOT frequency interleaved: do
 * not push it through frequency_map(). */
constexpr int16_t prs_carrier_at(size_t index) {
    return carrier_at_position(index);
}

/* 0..3, meaning exp(j*pi/2*value); 0 past the end. */
uint8_t prs_phase(size_t index);

/* By signed carrier index; 0xFF outside [-768, 768] or at DC. */
uint8_t prs_phase_of_carrier(int16_t k);

/* Multiplies by the conjugate reference and zeroes the 512 empty bins; the
 * inverse transform is the channel impulse response. `bin_shift` is added to
 * every bin modulo kFftSize: kFftSize/4 for the receiver's fs/4 front end, 0
 * for a raw HackRF capture. A wrong shift fails silently. */
void prs_conjugate_multiply(FftComplex* bins, size_t bin_shift = 0);

/* ---- Frequency interleaving ---- */

/* DAB frequency interleaving (EN 300 401 section 14.6): iterate
 * pi = (13 * pi + 511) mod 2048 and keep [256, 1792] except DC at 1024, exactly
 * 1536 residues, one per active carrier. */

/* `n` (must be 1536) carrier indices in [-768, 768] \ {0}, interleaved order.
 * Returns the number written, 0 on bad input. */
size_t build_frequency_map(int16_t* out, size_t n);

/* Built on first use; 1536 entries. */
const int16_t* frequency_map();

/* The inverse permutation, for dab_common.hpp's variant sweep. */
const int16_t* frequency_map_inverse();

/* Bin 0 is DC; negative carriers wrap to the top half. */
inline size_t carrier_to_fft_bin(int16_t carrier) {
    return static_cast<size_t>((carrier + 2048) & 2047);
}

/* ---- OFDM demodulation ---- */

/* OFDM front end for DAB mode I: null search, frequency offset, carrier
 * extraction and DQPSK demapping. Pure computation, so it builds on a host.
 * Soft bits: real parts are bits 0..1535, imaginary parts 1536..3071. */

/* Layout matches complex16_t; this header stays free of hardware includes. */
struct Iq16 {
    int16_t i{0};
    int16_t q{0};
};

/* The null symbol is the lowest-energy 2656-sample window. False if the buffer
 * is shorter or the minimum is not a convincing dip, i.e. no DAB signal. */
bool find_null_symbol(const Iq16* samples,
                      size_t count,
                      size_t& null_offset,
                      uint32_t& confidence_ratio);

/* Fractional offset in Hz from the guard's correlation with the tail it copies.
 * `symbol` is 2552 samples, guard first; valid within +/- 500 Hz. guard_quality
 * is the normalised magnitude in thousandths: ~1000 on a whole guard, ~0 when
 * the window straddles two symbols (the angle is then noise). */
float estimate_frequency_offset(const Iq16* symbol, uint16_t* guard_quality = nullptr);

/* Adds one symbol's guard correlation to a frame-long sum for
 * frequency_offset_from_sum(). Run on every data symbol, not the PRS. */
void accumulate_frequency_offset(const Iq16* symbol, int32_t* real_acc, int32_t* imag_acc);

/* Hz, from a sum over any number of symbols. */
float frequency_offset_from_sum(int32_t real_sum, int32_t imag_sum);

/* Rotates by -offset_hz in place. Float reference, too slow for the device:
 * the tests' oracle and offset injector. */
void correct_frequency_offset(Iq16* samples, size_t count, float offset_hz);

/* 32-bit phase accumulator, 8-bit sine table. Required: 0.5 ppm at Band III is
 * up to 120 Hz, ~54 degrees per symbol, enough to cross DQPSK boundaries. */
class FrequencyCorrector {
   public:
    /* Offset to remove, in Hz; restarts the phase. */
    void set_offset(float offset_hz);

    /* Continues the phase, so it stays coherent across symbols. */
    void apply(Iq16* samples, size_t count);

    void reset();

    /* Output scaled by 2^boost (the twiddles carry it, so the bits below the
     * input's LSB survive); apply() then runs even at zero offset. */
    void set_boost(uint8_t boost) { boost_ = boost; }

    bool active() const { return phase_increment_ != 0; }

   private:
    uint32_t phase_{0};
    int32_t phase_increment_{0};
    uint8_t boost_{0};
};

/* `bins`: FFT output, bin 0 at DC. `out`: kCarriers, frequency-deinterleaved. */
void extract_carriers(const Iq16* bins, Iq16* out);

/* 2 * count soft bits, real parts first. Positive means 1, as ViterbiDecoder
 * expects (the standard maps 0 to +1, so the correlation is negated).
 * Normalised per symbol, so a faded carrier stays smaller. */
void dqpsk_demap(const Iq16* previous, const Iq16* current, size_t count, int8_t* soft_out);

} /* namespace dab */

#endif /*__DAB_OFDM_H__*/
