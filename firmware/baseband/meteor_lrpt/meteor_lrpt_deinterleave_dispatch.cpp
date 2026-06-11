/*
 * Copyright (C) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "meteor_lrpt_deinterleave_dispatch.hpp"

#include "meteor_deinterleaver.hpp"

namespace meteor_lrpt {

int meteor_lrpt_deinterleave_dispatch(
    IMeteorDeintRing& ring,
    MeteorDeintFileRing* file_ring,
    int8_t* dst,
    const int8_t* src,
    size_t len,
    uint32_t& offset,
    int& cur_branch) {
    (void)file_ring;
    meteor_deinterleave(ring, dst, src, len, offset, cur_branch);
    return 0;
}

}  // namespace meteor_lrpt
