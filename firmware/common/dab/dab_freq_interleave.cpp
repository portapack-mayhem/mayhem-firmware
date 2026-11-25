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

#include "dab_freq_interleave.hpp"
#include "dab_constants.hpp"

namespace dab {

size_t build_frequency_map(int16_t* out, size_t n) {
    if (out == nullptr || n != kCarriers)
        return 0;

    size_t written = 0;
    uint32_t pi = 0;

    /* One full cycle of the affine map visits all 2048 residues exactly once. */
    for (size_t i = 0; i < 2048 && written < n; ++i) {
        pi = (13 * pi + 511) & 2047;
        if (pi >= 256 && pi <= 1792 && pi != 1024)
            out[written++] = static_cast<int16_t>(static_cast<int32_t>(pi) - 1024);
    }

    return written;
}

const int16_t* frequency_map_inverse() {
    static int16_t inverse[kCarriers];
    static bool built = false;
    if (!built) {
        const int16_t* forward = frequency_map();

        /* forward[k] is the carrier that logical index k reads. */
        static size_t perm_inv[kCarriers];
        for (size_t k = 0; k < kCarriers; ++k)
            perm_inv[carrier_position(forward[k])] = k;

        for (size_t k = 0; k < kCarriers; ++k)
            inverse[k] = carrier_at_position(perm_inv[k]);

        built = true;
    }
    return inverse;
}

const int16_t* frequency_map() {
    static int16_t map[kCarriers];
    static bool built = false;
    if (!built) {
        build_frequency_map(map, kCarriers);
        built = true;
    }
    return map;
}

} /* namespace dab */
