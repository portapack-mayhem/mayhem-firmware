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

/* Mode I phase reference symbol (EN 300 401 section 14.3), the only absolute
 * timing and frequency reference: correlated against it, the spectrum gives the
 * channel impulse response and the integer carrier offset. Phases are packed
 * two bits per carrier (384 bytes) at compile time from tables 43 and 44. */

#ifndef __DAB_PHASEREF_H__
#define __DAB_PHASEREF_H__

#include "dab_constants.hpp"
#include "dab_fft.hpp"

#include <cstddef>
#include <cstdint>

namespace dab {

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

} /* namespace dab */

#endif /*__DAB_PHASEREF_H__*/
