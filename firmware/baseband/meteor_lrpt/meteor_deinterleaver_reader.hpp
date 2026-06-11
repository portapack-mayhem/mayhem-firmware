/*
 * Copyright (C) 2026
 *
 * SatDump `DeinterleaverReader::read_samples` + ring FSM (plugins/meteor_support/meteor/deint.cpp).
 * `phase_t` / `rotate_soft` mapped to meteor_lrpt::rotate_soft_satdump (see meteor_soft_correlate.*).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef METEOR_LRPT_METEOR_DEINTERLEAVER_READER_HPP
#define METEOR_LRPT_METEOR_DEINTERLEAVER_READER_HPP

#include "meteor_deinterleaver.hpp"
#include "meteor_lrpt_ring_iface.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace meteor_lrpt {

class MeteorDeintFileRing;

inline constexpr size_t inter_size_bytes(size_t output_len) {
    return output_len * 10u / 9u + 8u;
}

/** Max `len` passed to `read_samples` on-device (8192-byte SatDump pulls). */
inline constexpr size_t kDeinterleaverReaderMaxOutputLen = 8192u;
inline constexpr size_t kDeinterleaverReaderHardScratchBytes = inter_size_bytes(kDeinterleaverReaderMaxOutputLen);

/** Stream read: return true if all `len` bytes were filled successfully. */
using MeteorDeintStreamRead = bool (*)(void* ctx, int8_t* buf, size_t len);

class MeteorDeinterleaverReader {
   public:
    /**
     * @param hard_scratch Byte buffer of at least `kDeinterleaverReaderHardScratchBytes` for the long
     *                     autocorrelation path (two M0 readers may share one buffer if `read_samples` never overlaps).
     * @param file_ring If non-null (M0 + `MeteorDeintFileRing`), use sector-batched deinterleave;
     *                  baseband / RAM rings pass nullptr.
     */
    explicit MeteorDeinterleaverReader(IMeteorDeintRing& ring, uint8_t* hard_scratch, MeteorDeintFileRing* file_ring = nullptr);

    /**
     * SatDump `read_samples` semantics: returns 0 on success, 1 on stream read failure,
     * 2 on SD deinterleave error (M0 file ring only).
     * `dst` must hold at least `deinterleave_num_samples(len)` bytes for the long path.
     */
    int read_samples(MeteorDeintStreamRead read, void* ctx, int8_t* dst, size_t len);

    uint32_t ring_offset() const { return ring_offset_; }
    int ring_branch() const { return ring_branch_; }
    unsigned rotation() const { return rotation_; }

   private:
    static int autocorrelate(unsigned* rotation_out, int period_bytes, uint8_t* hard, int len_bytes);

    IMeteorDeintRing& ring_;
    MeteorDeintFileRing* file_ring_{nullptr};
    uint32_t ring_offset_{0};
    int ring_branch_{0};
    int stream_carry_{0};
    std::array<int8_t, kInterMarkerStride> from_prev_{};
    unsigned rotation_{0};
    /** Caller-owned hard-decision scratch for long autocorr path (not heap). */
    uint8_t* hard_scratch_{nullptr};
};

}  // namespace meteor_lrpt

#endif
