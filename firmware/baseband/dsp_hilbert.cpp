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

#include "dsp_hilbert.hpp"
#include "dsp_sos_config.hpp"

namespace dsp {

HilbertTransform::HilbertTransform() {
    n = 0;

    sos_input.configure(half_band_lpf_config);
    sos_i.configure(half_band_lpf_config);
    sos_q.configure(half_band_lpf_config);
}

void HilbertTransform::execute(float in, float& out_i, float& out_q) {
    // Synthesized Hilbert Transform, it is implemented  based on 1/2 band LPF and later freq shift fs/4, achieving a H.T_BW of transmitted = fs/2 ;
    // Half_band LPF  means a LP filter with f_cut_off = fs/4; Half band = Half max band = 1/2 * fs_max =  1/2 x f_Nyquist = 1/2 * fs/2 = fs/4
    float a = 0, b = 0;

    float in_filtered = sos_input.execute(in) * 1.0f;  // Anti-aliasing LPF at fs/4 mic audio filter front-end.

    switch (n) {
        case 0:
            a = in_filtered;
            b = 0;
            break;
        case 1:
            a = 0;
            b = -in_filtered;
            break;
        case 2:
            a = -in_filtered;
            b = 0;
            break;
        case 3:
            a = 0;
            b = in_filtered;
            break;
    }

    float i = sos_i.execute(a) * 2.0f;
    float q = sos_q.execute(b) * 2.0f;

    switch (n) {
        case 0:
            out_i = i;
            out_q = q;
            break;
        case 1:
            out_i = -q;
            out_q = i;
            break;
        case 2:
            out_i = -i;
            out_q = -q;
            break;
        case 3:
            out_i = q;
            out_q = -i;
            break;
    }

    n = (n + 1) % 4;
}

} /* namespace dsp */
