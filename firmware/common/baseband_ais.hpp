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
constexpr std::array<std::complex<float>, 8> rrc_taps_8_n { {
	{  0.00533687f,  0.00000000f }, { -0.00667109f, -0.00667109f },
	{  0.00000000f, -0.01334218f }, { -0.05145006f,  0.05145006f },
	{ -0.14292666f,  0.00000000f }, { -0.05145006f, -0.05145006f },
	{  0.00000000f,  0.01334218f }, { -0.00667109f,  0.00667109f },
} };

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

constexpr std::array<std::complex<float>, 8> rrc_taps_8_n { {
	{  0.02104694f,  0.00000000f }, { -0.02630867f, -0.02630867f },
	{  0.00000000f, -0.05261734f }, { -0.20290270f,  0.20290270f },
	{ -0.56365746f,  0.00000000f }, { -0.20290270f, -0.20290270f },
	{  0.00000000f,  0.05261734f }, { -0.02630867f,  0.02630867f },
} };

constexpr std::array<std::complex<float>, 8> rrc_taps_8_p { {
	{  0.02104694f,  0.00000000f }, { -0.02630867f,  0.02630867f },
	{  0.00000000f,  0.05261734f }, { -0.20290270f, -0.20290270f },
	{ -0.56365746f,  0.00000000f }, { -0.20290270f,  0.20290270f },
	{  0.00000000f, -0.05261734f }, { -0.02630867f, -0.02630867f },
} };

constexpr std::array<std::complex<float>, 16> rrc_taps_16_n { {
	{ -0.00506828f,  0.00000000f }, {  0.00380121f,  0.00380121f },
	{  0.00000000f,  0.00152049f }, {  0.00532170f, -0.00532170f },
	{ -0.02128679f,  0.00000000f }, {  0.02660849f,  0.02660849f },
	{  0.00000000f,  0.05321698f }, {  0.20521503f, -0.20521503f },
	{  0.57008100f, -0.00000000f }, {  0.20521503f,  0.20521503f },
	{ -0.00000000f, -0.05321698f }, {  0.02660849f, -0.02660849f },
	{ -0.02128679f,  0.00000000f }, {  0.00532170f,  0.00532170f },
	{ -0.00000000f, -0.00152049f }, {  0.00380121f, -0.00380121f },
} };

constexpr std::array<std::complex<float>, 16> rrc_taps_16_p { {
	{ -0.00506828f,  0.00000000f }, {  0.00380121f, -0.00380121f },
	{  0.00000000f, -0.00152049f }, {  0.00532170f,  0.00532170f },
	{ -0.02128679f, -0.00000000f }, {  0.02660849f, -0.02660849f },
	{  0.00000000f, -0.05321698f }, {  0.20521503f,  0.20521503f },
	{  0.57008100f,  0.00000000f }, {  0.20521503f, -0.20521503f },
	{ -0.00000000f,  0.05321698f }, {  0.02660849f,  0.02660849f },
	{ -0.02128679f, -0.00000000f }, {  0.00532170f, -0.00532170f },
	{ -0.00000000f,  0.00152049f }, {  0.00380121f,  0.00380121f },
} };

constexpr std::array<std::complex<float>, 32> rrc_taps_32_n { {
	{ -0.00124950f,  0.00000000f }, {  0.00075863f,  0.00075863f },
	{  0.00000000f,  0.00011671f }, {  0.00087534f, -0.00087534f },
	{ -0.00222813f,  0.00000000f }, {  0.00144828f,  0.00144828f },
	{  0.00000000f,  0.00032184f }, {  0.00177013f, -0.00177013f },
	{ -0.00505750f,  0.00000000f }, {  0.00379313f,  0.00379313f },
	{  0.00000000f,  0.00151725f }, {  0.00531038f, -0.00531038f },
	{ -0.02124151f,  0.00000000f }, {  0.02655189f,  0.02655189f },
	{  0.00000000f,  0.05310377f }, {  0.20477849f, -0.20477849f },
	{  0.56886834f, -0.00000000f }, {  0.20477849f,  0.20477849f },
	{ -0.00000000f, -0.05310377f }, {  0.02655189f, -0.02655189f },
	{ -0.02124151f,  0.00000000f }, {  0.00531038f,  0.00531038f },
	{ -0.00000000f, -0.00151725f }, {  0.00379313f, -0.00379313f },
	{ -0.00505750f,  0.00000000f }, {  0.00177013f,  0.00177013f },
	{  0.00000000f, -0.00032184f }, {  0.00144828f, -0.00144828f },
	{ -0.00222813f,  0.00000000f }, {  0.00087534f,  0.00087534f },
	{ -0.00000000f, -0.00011671f }, {  0.00075863f, -0.00075863f },
} };

constexpr std::array<std::complex<float>, 32> rrc_taps_32_p { {
	{ -0.00124950f,  0.00000000f }, {  0.00075863f, -0.00075863f },
	{  0.00000000f, -0.00011671f }, {  0.00087534f,  0.00087534f },
	{ -0.00222813f, -0.00000000f }, {  0.00144828f, -0.00144828f },
	{  0.00000000f, -0.00032184f }, {  0.00177013f,  0.00177013f },
	{ -0.00505750f, -0.00000000f }, {  0.00379313f, -0.00379313f },
	{  0.00000000f, -0.00151725f }, {  0.00531038f,  0.00531038f },
	{ -0.02124151f, -0.00000000f }, {  0.02655189f, -0.02655189f },
	{  0.00000000f, -0.05310377f }, {  0.20477849f,  0.20477849f },
	{  0.56886834f,  0.00000000f }, {  0.20477849f, -0.20477849f },
	{ -0.00000000f,  0.05310377f }, {  0.02655189f,  0.02655189f },
	{ -0.02124151f, -0.00000000f }, {  0.00531038f, -0.00531038f },
	{ -0.00000000f,  0.00151725f }, {  0.00379313f,  0.00379313f },
	{ -0.00505750f, -0.00000000f }, {  0.00177013f, -0.00177013f },
	{  0.00000000f,  0.00032184f }, {  0.00144828f,  0.00144828f },
	{ -0.00222813f, -0.00000000f }, {  0.00087534f, -0.00087534f },
	{ -0.00000000f,  0.00011671f }, {  0.00075863f,  0.00075863f },
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
