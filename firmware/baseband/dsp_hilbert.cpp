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
#include "utility_m4.hpp"

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

Real_to_Complex::Real_to_Complex() {
    // No need to call a separate configuration method like "Real_to_Complex()" externally before using the execute() method
    // This is the constructor for the Real_to_Complex class.
    // It initializes the member variables and calls the configure function for the sos_input, sos_i, and sos_q filters.
    // to ensure the object is ready to use right after instantiation.

    n = 0;

    sos_input.configure(full_band_lpf_config);
    sos_i.configure(full_band_lpf_config);
    sos_q.configure(full_band_lpf_config);
    sos_mag_sq.configure(quarter_band_lpf_config);  // for APT LPF subcarrier filter. (1/4 Nyquist fs/2 = 1/4 * 12Khz/2 = 1.5khz)
}

void Real_to_Complex::execute(float in, float& out_mag_sq_lpf) {
    // Full_band LPF  means a LP filter with f_cut_off = fs/2; Full band = Full max band = 1/2 * fs_max =  1.0 x f_Nyquist = 1 * fs/2 = fs/2
    float a = 0, b = 0;
    float out_i = 0, out_q = 0, out_mag_sq = 0;
    // int32_t packed;

    float in_filtered = sos_input.execute(in) * 1.0f;  // Anti-aliasing full band LPF,  fc = fs/2= 6k, audio filter front-end.

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

    float i = sos_i.execute(a) * 1.0f;  // better keep <1.0f to minimize recorded APT(t) black level artifacts.-
    float q = sos_q.execute(b) * 1.0f;

    switch (n) {  // shifting down -fs4 (fs = 12khz , fs/4 = 3khz)
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

    /*   res = __smuad(val1,val2);  	p1 = val1[15:0] × val2[15:0]
                                                p2 = val1[31:16] × val2[31:16]
                                                res[31:0] = p1 + p2
                                        return res; */

    // Not strict Magnitude complex calculation, it is a cross multiplication (lower 16 bit real x lower 16 imag) + 0 (higher 16 bits comp),
    // but better visual results comparing real magnitude calculation, (better map diagonal lines reproduction, and less artifacts in APT signal(t)
    out_mag_sq = __SMUAD(out_i, out_q);                        // "cross-magnitude" of the complex (out_i + j out_q)
    out_mag_sq_lpf = sos_mag_sq.execute((out_mag_sq)) * 2.0f;  // LPF quater band = 1.5khz APT signal

    out_mag_sq_lpf /= 32768.0f;  // normalize ;
    // Compress clipping positive APT signal [-1.5 ..1.5] input , converted to [-1.0 ...1.0] with "S" compressor gain shape.
    if (out_mag_sq_lpf > 1.0f) {
        out_mag_sq_lpf = 1.0f;  // clipped signal  at +1.0f,  APT signal is positive, no need to clip -1.0
    } else {
        out_mag_sq_lpf = out_mag_sq_lpf * (1.5f - ((out_mag_sq_lpf * out_mag_sq_lpf) / 2.0f));
    }
}

} /* namespace dsp */
