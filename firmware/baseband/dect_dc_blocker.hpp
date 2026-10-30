/*
 * One-pole complex IIR DC blocker for the PortaPack Mayhem DECT RX.
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
 * Ported from DeDECTive dc_blocker.h. Removes the receiver DC spike that
 * otherwise sits in the middle of the DECT channel passband.
 */

#ifndef __DECT_DC_BLOCKER_H__
#define __DECT_DC_BLOCKER_H__

#include "dect_phase_diff.hpp"

namespace dect {

class DCBlocker {
   public:
    DCBlocker() { reset(); }

    inline cf_t process(cf_t x) noexcept {
        cf_t y;
        y.re = x.re - x_prev_.re + alpha * y_prev_.re;
        y.im = x.im - x_prev_.im + alpha * y_prev_.im;
        x_prev_ = x;
        y_prev_ = y;
        return y;
    }

    void reset() noexcept {
        x_prev_.re = x_prev_.im = 0.0f;
        y_prev_.re = y_prev_.im = 0.0f;
    }

   private:
    static constexpr float alpha = 0.9999f;
    cf_t x_prev_;
    cf_t y_prev_;
};

}  // namespace dect

#endif /*__DECT_DC_BLOCKER_H__*/
