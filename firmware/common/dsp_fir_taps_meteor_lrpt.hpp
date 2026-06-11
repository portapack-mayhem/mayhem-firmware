/*
 * Meteor LRPT — extra RX pulse shaping (complex channel, Fs = 192 kHz).
 * Taps: Hamming-windowed low-pass (~90 kHz @ 192 kHz) as a practical stand-in
 * for a full root-raised-cosine until offline remez-generated RRC is checked in.
 *
 * Mayhem-generated coefficients (not a SatDump upstream file); listed as Mayhem-only
 * in `firmware/baseband/meteor_lrpt/SATDUMP_VENDOR.md`.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef __DSP_FIR_TAPS_METEOR_LRPT_HPP__
#define __DSP_FIR_TAPS_METEOR_LRPT_HPP__

#include <array>
#include <cstddef>
#include <cstdint>

namespace meteor_lrpt {

/* Q13 fixed-point taps (sum of |tap| scaled ~8192); symmetric, length 17 */
constexpr size_t sym_shaping_ntaps = 17;
constexpr std::array<int16_t, sym_shaping_ntaps> sym_shaping_taps_q13{{
    0,
    6,
    -28,
    79,
    -166,
    283,
    -408,
    505,
    8126,
    505,
    -408,
    283,
    -166,
    79,
    -28,
    6,
    0,
}};

}  // namespace meteor_lrpt

#endif /* __DSP_FIR_TAPS_METEOR_LRPT_HPP__ */
