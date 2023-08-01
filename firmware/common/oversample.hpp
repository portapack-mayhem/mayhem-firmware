/*
 * Copyright (C) 2023 Kyle Reed, zxkmm
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

/* Helpers for handling oversampling and interpolation. */

#ifndef __OVERSAMPLE_H__
#define __OVERSAMPLE_H__

#include "message.hpp"
#include "utility.hpp"

/* TODO:
 * The decision to oversample/interpolate should only be a baseband concern.
 * However, the baseband can't set up the radio (M0 code), so the apps also
 * need to know about the "actual" sample rate so the radio settings can be
 * applied correctly. Ideally the baseband would tell the apps what the
 * actual sample rate is. Currently the apps are telling the baseband and
 * that feels like a separation of concerns problem. */

/* HackRF suggests a minimum sample rate of 2M so a oversample rate is applied
 * to the sample rate (pre-scale) to get the sample rate closer to that target.
 * The baseband needs to know how to correctly decimate (or interpolate) so
 * the set of allowed scalars is fixed (See OversampleRate enum). */


/* Gets the oversample rate for a given sample rate.
 * The oversample rate is used to increase the sample rate to improve SNR and quality.
 * This is also used as the interpolation rate when replaying captures. */
inline OversampleRate get_oversample_rate(uint32_t sample_rate) {
    // For lower bandwidths, (12k5, 16k, 20k), use the higher oversample rate.
    return sample_rate >= 25'000 ? OversampleRate::x8 : OversampleRate::x16;
}

/* Gets the actual sample rate for a given sample rate.
 * This is the rate with the correct oversampling rate applied. */
inline uint32_t get_actual_sample_rate(uint32_t sample_rate) {
    return sample_rate * toUType(get_oversample_rate(sample_rate));
}

#endif /*__OVERSAMPLE_H__*/
