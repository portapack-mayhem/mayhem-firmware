/*
 * Copyright (C) 2026
 *
 * Split dispatch for `meteor_deinterleave` vs FatFs `meteor_deinterleave_file_ring` so
 * `meteor_deinterleaver_reader.cpp` stays one TU compiled for M4 and M0 without M0-only includes.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef METEOR_LRPT_METEOR_LRPT_DEINTERLEAVE_DISPATCH_HPP
#define METEOR_LRPT_METEOR_LRPT_DEINTERLEAVE_DISPATCH_HPP

#include "meteor_lrpt_ring_iface.hpp"

#include <cstddef>
#include <cstdint>

namespace meteor_lrpt {

class MeteorDeintFileRing;

/**
 * Run SatDump-equivalent deinterleave into `dst` / ring state.
 * @param file_ring If non-null, use sector-batched SD path (`meteor_deinterleave_file_ring`).
 * @return 0 ok, 2 SD / file ring error (M0 only; baseband build never returns 2).
 */
int meteor_lrpt_deinterleave_dispatch(
    IMeteorDeintRing& ring,
    MeteorDeintFileRing* file_ring,
    int8_t* dst,
    const int8_t* src,
    size_t len,
    uint32_t& offset,
    int& cur_branch);

}  // namespace meteor_lrpt

#endif
