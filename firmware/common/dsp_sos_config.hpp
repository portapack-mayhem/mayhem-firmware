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

#ifndef __DSP_SOS_CONFIG_H__
#define __DSP_SOS_CONFIG_H__

#include "dsp_iir.hpp"

// scipy.signal.iirfilter(ftype="ellip", N = 10, rp = 0.5, rs = 60.0, Wn = 0.5, btype = 'lowpass', output="sos")
// 3khz cutt off  @fs:12Khz , used in Hilbert
constexpr iir_biquad_df2_config_t half_band_lpf_config[5] = {
    {0.02339042f, 0.0411599f, 0.02339042f, 1.0f, -0.95317621f, 0.33446485f},
    {1.0f, 0.82196114f, 1.0f, 1.0f, -0.50327735f, 0.63611027f},
    {1.0f, 0.32515305f, 1.0f, 1.0f, -0.18144446f, 0.85269598f},
    {1.0f, 0.14394122f, 1.0f, 1.0f, -0.04368236f, 0.94798064f},
    {1.0f, 0.08720754, 1.0f, 1.0f, 0.00220944f, 0.98743139f}};

// scipy.signal.iirfilter(ftype="ellip", N = 10, rp = 0.5, rs = 60.0, Wn = 0.99, btype = 'lowpass', output="sos")
// 6khz cutt off @fs:12Khz , used in WFAX demod.
constexpr iir_biquad_df2_config_t full_band_lpf_config[5] = {
    {0.88095275f, 1.76184993f, 0.88095275f, 1.0f, 1.89055677f, 0.89616378f},
    {1.0f, 1.99958798f, 1.0f, 1.0f, 1.9781807f, 0.98002549f},
    {1.0f, 1.99928911f, 1.0f, 1.0f, 1.99328036f, 0.99447816f},
    {1.0f, 1.99914562f, 1.0f, 1.0f, 1.997254f, 0.99828526f},
    {1.0f, 1.99909558f, 1.0f, 1.0f, 1.9986187f, 0.99960319f}};

// scipy.signal.iirfilter(ftype="ellip", N = 10, rp = 0.5 , rs = 60.0, Wn = 0.25, btype = 'lowpass', output="sos")
// 1.5khz cutt off  @fs:12Khz, used in WFAX demod.
constexpr iir_biquad_df2_config_t quarter_band_lpf_config[5] = {
    {0.00349312f, 0.00319397f, 0.00349312f, 1.0f, -1.53025211f, 0.6203438f},
    {1.0f, -0.83483341f, 1.0f, 1.0f, -1.47619047f, 0.77120659f},
    {1.0f, -1.23050154f, 1.0f, 1.0f, -1.43058949f, 0.9000896f},
    {1.0f, -1.33837384f, 1.0f, 1.0f, -1.41007744f, 0.96349953f},
    {1.0f, -1.36921549f, 1.0f, 1.0f, -1.40680439f, 0.9910884f}};

#endif /*__DSP_SOS_CONFIG_H__*/
