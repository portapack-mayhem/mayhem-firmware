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

#ifndef __DSP_SOS_H__
#define __DSP_SOS_H__

#include "dsp_iir.hpp"

#include <cstdint>
#include <cstddef>

template <size_t N>
class SOSFilter {

public:

    void configure(const iir_biquad_df2_config_t config[N]) {
        for (size_t i = 0; i < N; i++)
            filters[i].configure(config[i]);
    }

    float execute(float value) {
        for (auto &filter : filters)
            value = filter.execute(value);
        return value;
    }

private:

    IIRBiquadDF2Filter filters[N];
};

#endif/*__DSP_SOS_H__*/
