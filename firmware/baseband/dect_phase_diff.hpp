/*
 * GFSK phase-differential demodulator for the PortaPack Mayhem DECT RX.
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
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
 *
 * Ported from gr-dect2 phase_diff_impl.cc (Pavel Yazev) via DeDECTive.
 * Computes ph_diff = s[n-3] * conj(s[n]).
 *
 * The reference implementation returns atan2(imag, real). The receiver only
 * uses the sign of this value for the bit decision and its absolute value to
 * pick the best sampling point, both of which are preserved by the signed
 * imaginary part of the cross product. Returning that directly avoids an
 * atan2f per input sample, which would not fit the M4 budget at 4.608 Msps.
 *
 * With 4 samples/symbol this spans 3/4 of a bit period for DECT's BT=0.5 GFSK.
 */

#ifndef __DECT_PHASE_DIFF_H__
#define __DECT_PHASE_DIFF_H__

#include <cstddef>

namespace dect {

struct cf_t {
    float re;
    float im;
};

/* Cross-product of the current sample with the one 3 samples earlier
 * (s[n-3] * conj(s[n])). `im` drives the bit decision; `re` and `im` together
 * give the amplitude-independent angle used for symbol-timing recovery. */
struct phase_diff_t {
    float im;
    float re;
};

class PhaseDiff {
   public:
    PhaseDiff() { reset(); }

    /* Returns both components of the phase-difference cross product. No
     * division/sqrt per sample: the per-sample budget at 4.608 Msps is close
     * to the M4 limit, so the amplitude-normalised timing metric is computed
     * later, per burst, in find_best_smpl_point(). */
    inline phase_diff_t process(cf_t s) noexcept {
        history_[head_] = s;

        /* Sample 3 positions back in the 4-deep circular history. */
        const int prev = (head_ + 1) & 3;

        /* old * conj(current) = (ac + bd) + i(bc - ad) */
        const cf_t h = history_[prev];
        const float re = h.re * s.re + h.im * s.im;
        const float im = h.im * s.re - h.re * s.im;

        head_ = (head_ + 1) & 3;
        return {im, re};
    }

    void reset() noexcept {
        for (size_t i = 0; i < 4; i++) {
            history_[i].re = 0.0f;
            history_[i].im = 0.0f;
        }
        head_ = 0;
    }

   private:
    cf_t history_[4];
    int head_;
};

}  // namespace dect

#endif /*__DECT_PHASE_DIFF_H__*/
