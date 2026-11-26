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

/* Fixed-point 2048-point FFT, 16-bit complex, Q15 twiddles (dab_fft_r4.cpp).
 * It halves at stages 3, 5, 7 and 9 and saturates elsewhere, so the true
 * spectrum is output * 2^4. Halving every stage costs ~20 dB; halving later
 * saturates. */

#ifndef __DAB_FFT_H__
#define __DAB_FFT_H__

#include "dab_constants.hpp"

#include <cstddef>
#include <cstdint>

namespace dab {

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

} /* namespace dab */

#endif /*__DAB_FFT_H__*/
