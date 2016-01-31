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

// RRC length should be about 4 x the symbol length (4T) to do a good job of
// cleaning up ISI.

// Translate+RRC filter
// sample=38.4k, deviation=2400, b=0.5, symbol=9600
// Length: 16 taps, 4 symbols, 1 cycles of sinusoid
constexpr std::array<std::complex<float>, 16> rrc_taps_38k4_4t_p { {
	{  1.0619794019e-02f,  0.0000000000e+00f }, {  3.5758705854e-03f,  1.4811740938e-03f },
	{ -1.3274742629e-02f, -1.3274742629e-02f }, { -1.5018657262e-02f, -3.6258246051e-02f },
	{  0.0000000000e+00f, -2.6549484581e-02f }, { -1.5018657262e-02f,  3.6258246051e-02f },
	{ -1.0237997393e-01f,  1.0237997393e-01f }, { -2.2527985355e-01f,  9.3313970669e-02f },
	{ -2.8440842032e-01f,  0.0000000000e+00f }, { -2.2527985355e-01f, -9.3313970669e-02f },
	{ -1.0237997393e-01f, -1.0237997393e-01f }, { -1.5018657262e-02f, -3.6258246051e-02f },
	{  0.0000000000e+00f,  2.6549484581e-02f }, { -1.5018657262e-02f,  3.6258246051e-02f },
	{ -1.3274742629e-02f,  1.3274742629e-02f }, {  3.5758705854e-03f, -1.4811740938e-03f },
} };

} /* namespace ais */
} /* namespace baseband */

#endif/*__AIS_BASEBAND_H__*/
