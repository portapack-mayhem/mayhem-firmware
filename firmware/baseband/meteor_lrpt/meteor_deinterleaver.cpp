/*
 * Copyright (C) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "meteor_deinterleaver.hpp"

namespace meteor_lrpt {

void meteor_deinterleave(
    IMeteorDeintRing& ring,
    int8_t* dst,
    const int8_t* src,
    size_t len,
    uint32_t& offset,
    int& cur_branch) {
    const uint32_t ringb = (uint32_t)kDeinterleaverRingBytes;
    uint32_t read_idx = (offset + (uint32_t)kInterBranchCount * (uint32_t)kInterBranchDelay) % ringb;

    const int8_t* s = src;
    for (size_t i = 0; i < len; i++) {
        if (cur_branch == 0)
            s += 8;

        const int delay = (cur_branch % kInterBranchCount) * kInterBranchDelay * kInterBranchCount;
        const uint32_t write_idx = (uint32_t)((int64_t)offset - (int64_t)delay + (int64_t)ringb) % ringb;
        ring.write_byte(write_idx, *s++);
        offset = (offset + 1u) % ringb;
        cur_branch = (cur_branch + 1) % kInterMarkerIntersamps;
    }

    for (size_t i = 0; i < len; i++) {
        *dst++ = ring.read_byte(read_idx);
        read_idx = (read_idx + 1u) % ringb;
    }
}

}  // namespace meteor_lrpt
