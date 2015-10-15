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

#ifndef __BASEBAND_AIS_H__
#define __BASEBAND_AIS_H__

#include "crc.hpp"

#include <cstdint>
#include <cstddef>
#include <complex>
#include <array>
#include <bitset>

namespace baseband {

template<typename T, typename BitRemap>
class FieldReader {
public:
	constexpr FieldReader(
		const T& data
	) : data { data }
	{
	}

	uint32_t read(const size_t start_bit, const size_t length) {
		uint32_t value = 0;
		for(size_t i=start_bit; i<(start_bit + length); i++) {
			value = (value << 1) | data[bit_remap(i)];
		}
		return value;
	}

private:
	const T& data;
	const BitRemap bit_remap { };
};

namespace ais {

// RRC length should be about 4 x the symbol length to do a good job of
// cleaning up ISI.
/*
constexpr std::array<std::complex<float>, 8> rrc_taps_8_p { {
	{  0.00533687f,  0.00000000f }, { -0.00667109f,  0.00667109f },
	{  0.00000000f,  0.01334218f }, { -0.05145006f, -0.05145006f },
	{ -0.14292666f,  0.00000000f }, { -0.05145006f,  0.05145006f },
	{  0.00000000f, -0.01334218f }, { -0.00667109f, -0.00667109f },
} };
*/
//
// These came from GRC directly, and have a bit more gain, it appears.
//

constexpr std::array<std::complex<float>, 8> rrc_taps_8_p { {
	{  2.1046938375e-02f,  0.0000000000e+00f }, { -2.6308671336e-02f, -2.6308671336e-02f },
	{ -3.2218829289e-18f, -5.2617341280e-02f }, { -2.0290270482e-01f,  2.0290270482e-01f },
	{ -5.6365746260e-01f,  6.9028130738e-17f }, { -2.0290270482e-01f, -2.0290270482e-01f },
	{  9.6656487867e-18f,  5.2617341280e-02f }, { -2.6308671336e-02f,  2.6308671336e-02f },
} };

constexpr std::array<std::complex<float>, 16> rrc_taps_16_p { {
	{ -5.0682835281e-03f,  0.0000000000e+00f }, {  3.8012127427e-03f,  3.8012127427e-03f },
	{  9.3102863700e-20f,  1.5204851516e-03f }, {  5.3216978397e-03f, -5.3216978397e-03f },
	{ -2.1286793053e-02f,  2.6068802977e-18f }, {  2.6608490845e-02f,  2.6608490845e-02f },
	{  9.7758004319e-18f,  5.3216978908e-02f }, {  2.0521502845e-01f, -2.0521502845e-01f },
	{  5.7008099556e-01f, -1.3962957329e-16f }, {  2.0521502845e-01f,  2.0521502845e-01f },
	{ -1.6293000720e-17f, -5.3216978908e-02f }, {  2.6608490845e-02f, -2.6608490845e-02f },
	{ -2.1286793053e-02f,  7.8206408930e-18f }, {  5.3216978397e-03f,  5.3216978397e-03f },
	{ -6.5172004590e-19f, -1.5204851516e-03f }, {  3.8012127427e-03f, -3.8012127427e-03f },
} };

constexpr std::array<std::complex<float>, 32> rrc_taps_32_p { {
	{ -1.2495006667e-03f,  0.0000000000e+00f }, {  7.5862532786e-04f,  7.5862532786e-04f },
	{  7.1465238505e-21f,  1.1671159155e-04f }, {  8.7533694842e-04f, -8.7533694842e-04f },
	{ -2.2281305864e-03f,  2.7286729908e-19f }, {  1.4482847468e-03f,  1.4482847468e-03f },
	{  5.9121245638e-20f,  3.2184107113e-04f }, {  1.7701258199e-03f, -1.7701258199e-03f },
	{ -5.0575020723e-03f,  1.2387307449e-18f }, {  3.7931268039e-03f,  3.7931268039e-03f },
	{  4.6452407924e-19f,  1.5172507847e-03f }, {  5.3103774596e-03f, -5.3103774596e-03f },
	{ -2.1241510287e-02f,  7.8040042746e-18f }, {  2.6551890262e-02f,  2.6551890262e-02f },
	{  2.2761678735e-17f,  5.3103774786e-02f }, {  2.0477849246e-01f, -2.0477849246e-01f },
	{  5.6886833906e-01f, -2.7866511623e-16f }, {  2.0477849246e-01f,  2.0477849246e-01f },
	{ -2.9265015516e-17f, -5.3103774786e-02f }, {  2.6551890262e-02f, -2.6551890262e-02f },
	{ -2.1241510287e-02f,  1.3006673791e-17f }, {  5.3103774596e-03f,  5.3103774596e-03f },
	{ -3.7171317828e-18f, -1.5172507847e-03f }, {  3.7931268039e-03f, -3.7931268039e-03f },
	{ -5.0575020723e-03f,  3.7161922347e-18f }, {  1.7701258199e-03f,  1.7701258199e-03f },
	{  3.1551252346e-19f, -3.2184107113e-04f }, {  1.4482847468e-03f, -1.4482847468e-03f },
	{ -2.2281305864e-03f,  1.9100710935e-18f }, {  8.7533694842e-04f,  8.7533694842e-04f },
	{ -3.1451929164e-19f, -1.1671159155e-04f }, {  7.5862532786e-04f, -7.5862532786e-04f },
} };

struct BitRemap {
	size_t operator()(const size_t bit_index) const {
		return bit_index ^ 7;
	}
};

struct CRCBitRemap {
	size_t operator()(const size_t bit_index) const {
		return bit_index;
	}
};

using FieldReader = baseband::FieldReader<std::bitset<1024>, BitRemap>;
using CRCFieldReader = baseband::FieldReader<std::bitset<1024>, CRCBitRemap>;

struct PacketLengthRange {
	constexpr PacketLengthRange(
	) : min_bytes { 0 },
		max_bytes { 0 }
	{
	}

	constexpr PacketLengthRange(
		const uint16_t min_bits,
		const uint16_t max_bits
	) : min_bytes { static_cast<uint8_t>(min_bits / 8U) },
		max_bytes { static_cast<uint8_t>(max_bits / 8U) }
	{
		// static_assert((min_bits & 7) == 0, "minimum bits not a multiple of 8");
		// static_assert((max_bits & 7) == 0, "minimum bits not a multiple of 8");
	}

	bool contains(const size_t bit_count) const {
		return !is_above(bit_count) && !is_below(bit_count);
	}

	bool is_above(const size_t bit_count) const {
		return (min() > bit_count);
	}

	bool is_below(const size_t bit_count) const {
		return (max() < bit_count);
	}

	size_t min() const {
		return min_bytes * 8;
	}
	
	size_t max() const {
		return max_bytes * 8;
	}

private:
	const uint8_t min_bytes;
	const uint8_t max_bytes;
};

constexpr std::array<PacketLengthRange, 64> packet_length_range { {
	{    0,    0 },	// 0
	{  168,  168 },	// 1
	{  168,  168 },	// 2
	{  168,  168 }, // 3
	{  168,  168 }, // 4
	{  424,  424 }, // 5
	{    0,    0 }, // 6
	{    0,    0 }, // 7
	{    0, 1008 }, // 8
	{    0,    0 }, // 9
	{    0,    0 }, // 10
	{    0,    0 }, // 11
	{    0,    0 }, // 12
	{    0,    0 }, // 13
	{    0,    0 }, // 14
	{    0,    0 }, // 15
	{    0,    0 }, // 16
	{    0,    0 }, // 17
	{  168,  168 }, // 18
	{    0,    0 }, // 19
	{   72,  160 }, // 20
	{  272,  360 }, // 21
	{  168,  168 }, // 22
	{  160,  160 }, // 23
	{  160,  168 }, // 24
	{    0,  168 }, // 25
	{    0,    0 }, // 26
	{    0,    0 }, // 27
	{    0,    0 }, // 28
	{    0,    0 }, // 29
	{    0,    0 }, // 30
	{    0,    0 }, // 31
} };

struct PacketLengthValidator {
	bool operator()(const uint_fast8_t message_id, const size_t length) {
		return packet_length_range[message_id].contains(length);
	}
};

struct PacketTooLong {
	bool operator()(const uint_fast8_t message_id, const size_t length) {
		return packet_length_range[message_id].is_below(length);
	}
};

struct CRCCheck {
	bool operator()(const std::bitset<1024>& payload, const size_t data_length) {
		CRCFieldReader field_crc { payload };
		CRC<uint16_t> ais_fcs { 0x1021 };
		
		uint16_t crc_calculated = 0xffff;
		
		for(size_t i=0; i<data_length + 16; i+=8) {
			if( i == data_length ) {
				crc_calculated ^= 0xffff;
			}
			const uint8_t byte = field_crc.read(i, 8);
			crc_calculated = ais_fcs.calculate_byte(crc_calculated, byte);
		}

		return crc_calculated == 0x0000;
	}
};

} /* namespace ais */
} /* namespace baseband */

#endif/*__BASEBAND_AIS_H__*/
