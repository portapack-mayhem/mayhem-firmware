/*
 * Copyright (C) 2026
 *
 * SatDump `DeinterleaverReader::deinterleave` (plugins/meteor_support/meteor/deint.cpp) ported
 * to use IMeteorDeintRing. Pin: see SATDUMP_VENDOR.md.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef METEOR_LRPT_METEOR_DEINTERLEAVER_HPP
#define METEOR_LRPT_METEOR_DEINTERLEAVER_HPP

#include "meteor_lrpt_ring_iface.hpp"

#include <cstddef>
#include <cstdint>

namespace meteor_lrpt {

/* SatDump deint.h */
inline constexpr int kInterBranchCount = 36;
inline constexpr int kInterBranchDelay = 2048;
inline constexpr int kInterMarkerStride = 80;
inline constexpr int kInterMarkerIntersamps = kInterMarkerStride - 8; /* INTER_MARKER_INTERSAMPS */

inline constexpr size_t meteor_deinterleave_num_samples(size_t output_count, int cur_branch) {
    if (!output_count)
        return 0;
    const int num_syncs = (cur_branch ? 0 : 1) +
                          (int)((output_count - (size_t)(kInterMarkerIntersamps - cur_branch) + (size_t)kInterMarkerIntersamps - 1) /
                                (size_t)kInterMarkerIntersamps);
    return output_count + 8u * (size_t)num_syncs;
}

inline int meteor_deinterleave_expected_sync_offset(int cur_branch) {
    return cur_branch ? (kInterMarkerIntersamps - cur_branch) : 0;
}

/**
 * Same semantics as SatDump `DeinterleaverReader::deinterleave(dst, src, len)`:
 * `read_idx` captured from `_offset` before writes; `len` writes then `len` sequential reads.
 * `offset` / `cur_branch` are in-out FSM state (mod ring / mod 72).
 */
void meteor_deinterleave(
    IMeteorDeintRing& ring,
    int8_t* dst,
    const int8_t* src,
    size_t len,
    uint32_t& offset,
    int& cur_branch);

}  // namespace meteor_lrpt

#endif
