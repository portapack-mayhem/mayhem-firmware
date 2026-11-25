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

/* OFDM front end for DAB mode I: null search, frequency offset, carrier
 * extraction and DQPSK demapping. Pure computation, so it builds on a host.
 * Soft bits: real parts are bits 0..1535, imaginary parts 1536..3071. */

#ifndef __DAB_OFDM_H__
#define __DAB_OFDM_H__

#include "dab_constants.hpp"

#include <cstddef>
#include <cstdint>

namespace dab {

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
