/*
 * Copyright (C) 2020 Belousov Oleg
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

#ifndef __DSP_HILBERT_H__
#define __DSP_HILBERT_H__

#include "dsp_sos.hpp"
#include "dsp_types.hpp"

namespace dsp {

class HilbertTransform {
   public:
    HilbertTransform();
    void execute(float in, float& out_i, float& out_q);

   private:
    uint8_t n = 0;
    SOSFilter<5> sos_input = {};
    SOSFilter<5> sos_i = {};
    SOSFilter<5> sos_q = {};
};

class Real_to_Complex {
   public:
    Real_to_Complex();  // Additional initialization
    void execute(float in, float& out_mag_sq_lpf);

   private:
    uint8_t n = 0;
    SOSFilter<5> sos_input = {};
    SOSFilter<5> sos_i = {};
    SOSFilter<5> sos_q = {};
    SOSFilter<5> sos_mag_sq = {};
};

} /* namespace dsp */

#endif /*__DSP_HILBERT_H__*/
