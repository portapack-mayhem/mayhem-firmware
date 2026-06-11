/*
 * Copyright (C) 2026
 *
 * Shared deinterleaver ring interface (M4 baseband + M0 application).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef METEOR_LRPT_RING_IFACE_HPP
#define METEOR_LRPT_RING_IFACE_HPP

#include <cstddef>
#include <cstdint>

namespace meteor_lrpt {

inline constexpr size_t kDeinterleaverRingBytes = 36u * 36u * 2048u; /* SatDump deint.h */
inline constexpr size_t kDeinterleaverSectorBytes = 512u;         /* FatFs _MIN_SS */

class IMeteorDeintRing {
   public:
    virtual ~IMeteorDeintRing() = default;
    virtual void write_byte(uint32_t index_mod, int8_t v) = 0;
    virtual int8_t read_byte(uint32_t index_mod) const = 0;
};

}  // namespace meteor_lrpt

#endif
