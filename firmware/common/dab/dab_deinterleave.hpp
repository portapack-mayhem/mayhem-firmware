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

/* DAB MSC time deinterleaving (EN 300 401 12.1): bit i of a CIF's subchannel is
 * on branch i % 16, delayed bitreverse4(branch) CIFs at the transmitter and the
 * complementary 15 - that here, so every branch lines up 15 CIFs late. Storage
 * is 7.5 three-bit codes (dab_soft.hpp) per coded bit. The FIC is not time
 * interleaved. */

#ifndef __DAB_DEINTERLEAVE_H__
#define __DAB_DEINTERLEAVE_H__

#include "dab_constants.hpp"
#include "dab_soft.hpp"

#include <cstddef>
#include <cstdint>

namespace dab {

/* Branches must divide the CIF evenly. */
constexpr size_t kInterleaverBranches = 16;

/* Total delay applied across transmitter and receiver, in CIFs. */
constexpr size_t kInterleaverLatencyCifs = 15;

class TimeDeinterleaver {
   public:
    TimeDeinterleaver() = default;

    /* `bits_per_cif` must be a non-zero multiple of 16, at most
     * kMaxCodedBitsPerCif. `storage` holds storage_bytes_for() bytes, outlives
     * this, and on the M4 is not the AHB "USB" bank, which overlaps M0 RAM. */
    bool configure(size_t bits_per_cif, uint8_t* storage, size_t storage_bytes);

    /* Codes at three bits, eight to three bytes. Each branch row is padded to
     * whole groups of eight; the slack after the last row is never read.
     * configure() refuses a smaller buffer. */
    static constexpr size_t row_slots(size_t bits_per_cif) {
        return ((bits_per_cif / kInterleaverBranches) + 7) & ~static_cast<size_t>(7);
    }
    static constexpr size_t storage_bytes_for(size_t bits_per_cif) {
        return row_slots(bits_per_cif) * 120 / 8 * 3;
    }

    /* One packed CIF, in place: on return `cif` holds the deinterleaved
     * codes. False for the first 15 calls, while the pipeline fills. */
    bool push_packed(uint8_t* cif);

    void reset();

    size_t bits_per_cif() const { return bits_per_cif_; }

    /* Transmitter-side delay for a branch, in CIFs. Exposed so tests can
     * build the matching interleaver. */
    static uint8_t branch_delay(uint8_t branch);

   private:
    size_t bits_per_cif_{0};
    size_t per_branch_{0};
    size_t row_{0};
    size_t offset_[kInterleaverBranches]{};
    size_t head_[kInterleaverBranches]{};
    uint8_t depth_[kInterleaverBranches]{};
    size_t pushed_{0};

    uint8_t* storage_{nullptr};
    size_t storage_bytes_{0};

    /* Advances every branch's head; true once the pipeline has filled. */
    inline bool end_cif();
};

} /* namespace dab */

#endif /*__DAB_DEINTERLEAVE_H__*/
