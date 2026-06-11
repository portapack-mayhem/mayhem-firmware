/*
 * Copyright (C) 2026
 *
 * This file is part of PortaPack / Mayhem.
 *
 * QPSK sync word constants match SatDump `module_meteor_lrpt_decoder.cpp` (MIT).
 * Upstream mapping: `SATDUMP_VENDOR.md`.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef __METEOR_SOFT_CORRELATE_HPP__
#define __METEOR_SOFT_CORRELATE_HPP__

#include <cstddef>
#include <cstdint>

namespace meteor_lrpt {

/* SatDump METEOR LRPT legacy QPSK sync words (module_meteor_lrpt_decoder.cpp). */
constexpr uint64_t kLrptSyncNoDiff = 0xfca2b63db00d9794ULL;
constexpr uint64_t kLrptSyncDiff = 0xfc4ef4fd0cc2df89ULL;

struct CorrQpskResult {
    size_t align_skip{0}; /* int8 soft symbols dropped from front (SatDump correlate return) */
    int score{0};         /* 0..64 Hamming agreement on 64-bit sync window */
    bool locked{false};  /* true if first-window correlation exceeded threshold (SatDump pos==0 sense) */
    unsigned phase{0};    /* 0..3 = PHASE_0..PHASE_270 */
    bool swap{false};
};

/* Pack int8 softs to hard bits (soft>0), find best QPSK sync, memmove+zero-pad tail, rotate I/Q pairs.
 * `length` must be a multiple of 8 (16384 for LRPT). */
void correlate_rotate_qpsk_legacy(int8_t* soft_io, size_t length, bool diff_decode, CorrQpskResult* out);

/* SatDump `rotate_soft` on int8 I/Q pairs: phase 0..3, optional IQ swap (see rotation.cpp). */
void rotate_soft_satdump(int8_t* soft_io, size_t nbytes, unsigned phase_mod4, bool iq_swap);

/* SatDump DintSampleReader path2: copy then rotate_soft(..., PHASE_90, false) on int8 I/Q pairs. */
void copy_soft_phase90(const int8_t* src, int8_t* dst, size_t nbytes);

}  // namespace meteor_lrpt

#endif /* __METEOR_SOFT_CORRELATE_HPP__ */
