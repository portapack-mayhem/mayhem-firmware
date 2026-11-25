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

/* RS(120,110) for DAB+ (TS 102 563): GF(256), primitive polynomial 0x11D,
 * g(x) = product of (x - a^i), i = 0..9; t = 5. Shortened RS(255,245): the 135
 * leading bytes are zero and never transmitted. */

#ifndef __DAB_REED_SOLOMON_H__
#define __DAB_REED_SOLOMON_H__

#include "dab_constants.hpp"

#include <cstddef>
#include <cstdint>

namespace dab {

constexpr size_t kRsN = kRsBlockLength;
constexpr size_t kRsK = kRsDataLength;
constexpr size_t kRsParity = kRsN - kRsK;
constexpr size_t kRsT = kRsParity / 2;

class ReedSolomon {
   public:
    ReedSolomon();

    /* In place. Returns errors corrected, or -1 if uncorrectable; the block
     * may then be partly modified. */
    int decode(uint8_t block[kRsN]) const;

    /* Fills block[110..119] from the data bytes. Tests only. */
    void encode(uint8_t block[kRsN]);

   private:
    uint8_t exp_[512];
    uint8_t log_[256];
    uint8_t generator_[kRsParity + 1];

    /* mul(v, a^i) for every v, one table per syndrome (2560 bytes): decode()
     * takes all the syndromes in one pass, a table read per byte each. */
    uint8_t root_mul_[kRsParity][256];

    uint8_t mul(uint8_t a, uint8_t b) const {
        if (a == 0 || b == 0)
            return 0;
        return exp_[log_[a] + log_[b]];
    }

    uint8_t div(uint8_t a, uint8_t b) const {
        if (a == 0)
            return 0;
        return exp_[log_[a] + 255 - log_[b]];
    }

    uint8_t pow_alpha(int e) const {
        e %= 255;
        if (e < 0)
            e += 255;
        return exp_[e];
    }
};

} /* namespace dab */

#endif /*__DAB_REED_SOLOMON_H__*/
