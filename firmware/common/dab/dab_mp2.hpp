/*
 * Copyright (C) 2026 Mayhem firmware contributors
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

/* MPEG-1/2 Layer II for classic DAB, wrapping vendor/kjmp2. One logical frame
 * is one Layer II frame (1152 samples, 24 ms at 48 kHz). kjmp2 emits stereo;
 * the mono path averages the channels so a hard-panned source is not lost. */

#ifndef __DAB_MP2_H__
#define __DAB_MP2_H__

/* kjmp2 is vendored unmodified and has no extern "C" guard of its own. */
extern "C" {
#include "vendor/kjmp2.h"
}

#include <cstddef>
#include <cstdint>

namespace dab {

constexpr size_t kMp2SamplesPerFrame = KJMP2_SAMPLES_PER_FRAME;  // 1152

class Mp2Decoder {
   public:
    Mp2Decoder() { reset(); }

    void reset();

    /* 0 if not a valid Layer II header; needs eight bytes. */
    static uint32_t frame_sample_rate(const uint8_t* frame, size_t length);

    /* Returns mono samples, 0 if unusable. Downmixed in place over the front of
     * the stereo buffer, read via pcm(): a separate array would cost 2.3 KB. */
    size_t decode(const uint8_t* frame, size_t length);

    const int16_t* pcm() const { return stereo_; }

    /* Bytes the header declares, 0 if invalid. Not static: it reuses context_. */
    size_t frame_length(const uint8_t* frame, size_t length) const;

   private:
    mutable kjmp2_context_t context_{};
    int16_t stereo_[kMp2SamplesPerFrame * 2]{};
};

} /* namespace dab */

#endif /*__DAB_MP2_H__*/
