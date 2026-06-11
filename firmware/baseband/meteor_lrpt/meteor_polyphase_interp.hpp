/*
 * Copyright (C) 2026
 *
 * Catmull–Rom cubic segment between the middle two control points (polyphase-style
 * fractional delay). Used for Meteor LRPT symbol strobes after RRC + AGC.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef METEOR_LRPT_METEOR_POLYPHASE_INTERP_HPP
#define METEOR_LRPT_METEOR_POLYPHASE_INTERP_HPP

namespace meteor_lrpt {

/**
 * Catmull–Rom spline on uniform knots, segment between `p1` and `p2` (inclusive ends).
 * @param t 0 → `p1`, 1 → `p2`
 */
inline float catmull_rom_segment(float p0, float p1, float p2, float p3, float t) {
    const float t2 = t * t;
    const float t3 = t2 * t;
    return 0.5f * ((2.f * p1) + (-p0 + p2) * t + (2.f * p0 - 5.f * p1 + 4.f * p2 - p3) * t2 +
                   (-p0 + 3.f * p1 - 3.f * p2 + p3) * t3);
}

/** One-sided extrapolation for missing post-cursor sample (keeps tangent smooth). */
inline float extrap_next_sample(float pm1, float p0, float p1) {
    (void)pm1;
    return 2.f * p1 - p0;
}

}  // namespace meteor_lrpt

#endif
