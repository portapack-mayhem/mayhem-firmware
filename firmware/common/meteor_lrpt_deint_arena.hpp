/*
 * Copyright (C) 2026
 *
 * M0 AHB SRAM layout for Meteor LRPT interleaved deinterleave (IPC-visible buffers).
 * Do not place in local_sram_0: M4 Meteor uses Bank1 for soft capture .bss.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef FIRMWARE_COMMON_METEOR_LRPT_DEINT_ARENA_HPP
#define FIRMWARE_COMMON_METEOR_LRPT_DEINT_ARENA_HPP

#include "meteor_lrpt/meteor_deinterleaver.hpp"
#include "meteor_lrpt/meteor_deinterleaver_reader.hpp"

#include <cstddef>
#include <cstdint>

namespace meteor_lrpt {

inline constexpr size_t kMeteorDeintSoftBlockBytes = 8192u;

inline constexpr size_t kMeteorDeintBranchBWorkBytes =
    meteor_deinterleave_num_samples(kMeteorDeintSoftBlockBytes, 0) + (size_t)(kInterMarkerStride / 2) + 32u;

struct alignas(64) MeteorDeintBlockBuffers {
    int8_t soft_in[kMeteorDeintSoftBlockBytes];
    int8_t deint_a[kMeteorDeintSoftBlockBytes];
    int8_t deint_b[kMeteorDeintSoftBlockBytes];
    int8_t branch_b_work[kMeteorDeintBranchBWorkBytes];
    uint8_t reader_hard[kDeinterleaverReaderHardScratchBytes];
};

struct MeteorDeintArenaStorage {
    MeteorDeintBlockBuffers bufs;
    alignas(MeteorDeinterleaverReader) unsigned char reader_a_blob[sizeof(MeteorDeinterleaverReader)];
    alignas(MeteorDeinterleaverReader) unsigned char reader_b_blob[sizeof(MeteorDeinterleaverReader)];
};

inline constexpr size_t kMeteorDeintArenaBytes = sizeof(MeteorDeintArenaStorage);

}  // namespace meteor_lrpt

#endif /* FIRMWARE_COMMON_METEOR_LRPT_DEINT_ARENA_HPP */
