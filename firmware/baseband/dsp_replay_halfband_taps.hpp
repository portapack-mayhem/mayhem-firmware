// Frozen Q15 coefficients for Replay HB47/HB11 interpolation.
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DSP_REPLAY_HALFBAND_TAPS_HPP
#define DSP_REPLAY_HALFBAND_TAPS_HPP
#include <array>
#include <cstdint>
namespace dsp {
namespace replay {
// Unique symmetric Q15 pairs, nearest center first. Odd-indexed taps are zero
// except the center, a delayed copy with implicit Q15 coefficient 32768.
// Both interpolation phases have unity DC gain: the filtered branch sums to
// 32768 and the delayed-copy phase has gain 32768. The prototype sums to 65536.
// Response figures below describe individual FIR prototypes, before repetition.
// HB47: 24 stored-branch nonzero taps, 12 unique pairs.
// Edges / stage output Fs: pass=0.2125, stop=0.2875.
// Q15 ripple=0.012143 dB; rejection=63.001 dB.
constexpr std::array<int16_t, 12> hb47_pairs{{20774, -6692, 3753, -2413, 1632, -1110, 751, -490, 311, -180, 100, -52}};
// HB11: 6 stored-branch nonzero taps, 3 unique pairs.
// Edges / stage output Fs: pass=0.1125, stop=0.3875.
// Q15 ripple=0.016364 dB; rejection=60.430 dB.
constexpr std::array<int16_t, 3> hb11_pairs{{19639, -3954, 699}};
}  // namespace replay
}  // namespace dsp
#endif
