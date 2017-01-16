/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __AIS_BASEBAND_H__
#define __AIS_BASEBAND_H__

#include <cstdint>
#include <cstddef>
#include <complex>
#include <array>
#include <bitset>
#include <utility>

namespace baseband {
namespace ais {

// Translate+Rectangular window filter
// sample=38.4k, deviation=2400, symbol=9600
// Length: 4 taps, 1 symbol, 1/4 cycle of sinusoid
// Gain: 1.0 (sinusoid / len(taps))
constexpr std::array<std::complex<float>, 4> square_taps_38k4_1t_p { {
	{ 0.25000000f, 0.00000000f }, { 0.23096988f, 0.09567086f },
	{ 0.17677670f, 0.17677670f }, { 0.09567086f, 0.23096988f },
} };

} /* namespace ais */
} /* namespace baseband */

#endif/*__AIS_BASEBAND_H__*/
