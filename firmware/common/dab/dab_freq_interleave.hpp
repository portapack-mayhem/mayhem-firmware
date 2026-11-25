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

/* DAB frequency interleaving (EN 300 401 section 14.6): iterate
 * pi = (13 * pi + 511) mod 2048 and keep [256, 1792] except DC at 1024, exactly
 * 1536 residues, one per active carrier. */

#ifndef __DAB_FREQ_INTERLEAVE_H__
#define __DAB_FREQ_INTERLEAVE_H__

#include <cstddef>
#include <cstdint>

namespace dab {

/* `n` (must be 1536) carrier indices in [-768, 768] \ {0}, interleaved order.
 * Returns the number written, 0 on bad input. */
size_t build_frequency_map(int16_t* out, size_t n);

/* Built on first use; 1536 entries. */
const int16_t* frequency_map();

/* The inverse permutation, for dab_variant.hpp's variant sweep. */
const int16_t* frequency_map_inverse();

/* Bin 0 is DC; negative carriers wrap to the top half. */
inline size_t carrier_to_fft_bin(int16_t carrier) {
    return static_cast<size_t>((carrier + 2048) & 2047);
}

} /* namespace dab */

#endif /*__DAB_FREQ_INTERLEAVE_H__*/
