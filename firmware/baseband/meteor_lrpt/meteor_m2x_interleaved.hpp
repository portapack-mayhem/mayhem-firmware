/*
 * Copyright (C) 2026
 *
 * Building blocks for SatDump interleaved M2-x dual soft streams (module_meteor_lrpt_decoder.cpp).
 * Full `Viterbi1_2` port is tracked separately — see meteor_viterbi12_stub.hpp.
 * Soft sample alignment / `read_samples` path: meteor_deinterleaver_reader.hpp.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef METEOR_LRPT_METEOR_M2X_INTERLEAVED_HPP
#define METEOR_LRPT_METEOR_M2X_INTERLEAVED_HPP

#include "meteor_soft_correlate.hpp"

#include <cstring>

namespace meteor_lrpt {

/** SatDump `DintSampleReader::read2` path: PHASE_90 rotate on a copy of the last 8192-byte chunk. */
inline void m2x_duplicate_phase90_chunk(const int8_t* src_chunk, int8_t* dst_chunk, size_t nbytes) {
    copy_soft_phase90(src_chunk, dst_chunk, nbytes);
}

}  // namespace meteor_lrpt

#endif
