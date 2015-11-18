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
// sample=76.8k, deviation=2400, b=0.5, symbol=9600
// Length: 32 taps, 4 symbols, 1 cycle of sinusoid
constexpr std::array<std::complex<float>, 32> rrc_taps_76k8_4t_p { {
	{  5.3368736990e-03f,  0.0000000000e+00f }, {  4.6850211151e-03f,  9.3190864122e-04f },
	{  1.7970187432e-03f,  7.4434953528e-04f }, { -2.5478608559e-03f, -1.7024261963e-03f },
	{ -6.6710920858e-03f, -6.6710920858e-03f }, { -8.6960320791e-03f, -1.3014531722e-02f },
	{ -7.5474785839e-03f, -1.8221225159e-02f }, { -3.8115552023e-03f, -1.9161981995e-02f },
	{ -8.1697309033e-19f, -1.3342183083e-02f }, {  3.6940784220e-05f, -1.8571386338e-04f },
	{ -7.5474785839e-03f,  1.8221225159e-02f }, { -2.4954265902e-02f,  3.7346698152e-02f },
	{ -5.1450054092e-02f,  5.1450054092e-02f }, { -8.3018814119e-02f,  5.5471398140e-02f },
	{ -1.1321218147e-01f,  4.6894020990e-02f }, { -1.3498960022e-01f,  2.6851100952e-02f },
	{ -1.4292666316e-01f,  1.7503468055e-17f }, { -1.3498960022e-01f, -2.6851100952e-02f },
	{ -1.1321218147e-01f, -4.6894020990e-02f }, { -8.3018814119e-02f, -5.5471398140e-02f },
	{ -5.1450054092e-02f, -5.1450054092e-02f }, { -2.4954265902e-02f, -3.7346698152e-02f },
	{ -7.5474785839e-03f, -1.8221225159e-02f }, {  3.6940784220e-05f,  1.8571386338e-04f },
	{  2.4509192710e-18f,  1.3342183083e-02f }, { -3.8115552023e-03f,  1.9161981995e-02f },
	{ -7.5474785839e-03f,  1.8221225159e-02f }, { -8.6960320791e-03f,  1.3014531722e-02f },
	{ -6.6710920858e-03f,  6.6710920858e-03f }, { -2.5478608559e-03f,  1.7024261963e-03f },
	{  1.7970187432e-03f, -7.4434953528e-04f }, {  4.6850211151e-03f, -9.3190864122e-04f },
} };

using decoded_packet = std::pair<std::string, std::string>;

decoded_packet packet_decode(const std::bitset<1024>& data, const size_t data_length);

} /* namespace ais */
} /* namespace baseband */

#endif/*__AIS_BASEBAND_H__*/
