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

#include "dab_mp2.hpp"

namespace dab {

void Mp2Decoder::reset() {
    kjmp2_init(&context_);
}

uint32_t Mp2Decoder::frame_sample_rate(const uint8_t* frame, size_t length) {
    /* The rate is in the first three bytes, but the frame-size parse this gates
     * reads to about frame[7]. */
    if (frame == nullptr || length < 8)
        return 0;

    const int rate = kjmp2_get_sample_rate(frame);
    return (rate > 0) ? static_cast<uint32_t>(rate) : 0;
}

size_t Mp2Decoder::frame_length(const uint8_t* frame, size_t length) const {
    if (frame_sample_rate(frame, length) == 0)
        return 0;

    /* A null output returns the frame size without touching state. The live
     * context is reused: a local one is 8200 bytes against a 4 KB stack. */
    const unsigned long declared = kjmp2_decode_frame(&context_, frame, nullptr);
    return static_cast<size_t>(declared);
}

size_t Mp2Decoder::decode(const uint8_t* frame, size_t length) {
    if (frame == nullptr)
        return 0;

    /* kjmp2 does no bounds checking: it reads what the header declares. */
    const size_t declared = frame_length(frame, length);
    if (declared == 0 || declared > length)
        return 0;

    if (kjmp2_decode_frame(&context_, frame, stereo_) == 0)
        return 0;

    /* Mono sample i lands on pair i's left slot, so nothing is overwritten early. */
    for (size_t i = 0; i < kMp2SamplesPerFrame; i++) {
        const int32_t l = stereo_[2 * i];
        const int32_t r = stereo_[2 * i + 1];
        stereo_[i] = static_cast<int16_t>((l + r) / 2);
    }

    return kMp2SamplesPerFrame;
}

} /* namespace dab */
