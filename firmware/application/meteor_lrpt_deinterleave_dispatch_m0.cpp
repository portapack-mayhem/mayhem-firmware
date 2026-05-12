/*
 * Copyright (C) 2026
 *
 * M0-only strong definition of `meteor_lrpt::meteor_lrpt_deinterleave_dispatch` (FatFs path).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "meteor_lrpt/meteor_lrpt_deinterleave_dispatch.hpp"

#include "meteor_lrpt/meteor_deinterleaver.hpp"
#include "meteor_lrpt_sector_file_ring.hpp"

namespace meteor_lrpt {

int meteor_lrpt_deinterleave_dispatch(
    IMeteorDeintRing& ring,
    MeteorDeintFileRing* file_ring,
    int8_t* dst,
    const int8_t* src,
    size_t len,
    uint32_t& offset,
    int& cur_branch) {
    if (file_ring) {
        const auto err = meteor_deinterleave_file_ring(*file_ring, dst, src, len, offset, cur_branch);
        return err.is_valid() ? 2 : 0;
    }
    meteor_deinterleave(ring, dst, src, len, offset, cur_branch);
    return 0;
}

}  // namespace meteor_lrpt
