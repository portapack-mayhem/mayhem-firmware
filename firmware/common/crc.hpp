/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __CRC_H__
#define __CRC_H__

#include <cstddef>
#include <cstdint>
#include <limits>
#include <array>

/* Inspired by
 * http://www.barrgroup.com/Embedded-Systems/How-To/CRC-Calculation-C-Code
 *
 * ...then munged into a simplified implementation of boost::crc_basic and
 * boost::crc_optimal.
 * http://www.boost.org/doc/libs/release/libs/crc/
 *
 *  Copyright 2001, 2004 Daryle Walker.  Use, modification, and distribution are
 *  subject to the Boost Software License, Version 1.0.  (See accompanying file
 *  LICENSE_1_0.txt or a copy at <http://www.boost.org/LICENSE_1_0.txt>.)
 *
 */

template<size_t Width, bool RevIn = false, bool RevOut = false>
class CRC {
public:
	using value_type = uint32_t;

	constexpr CRC(
		const value_type truncated_polynomial,
		const value_type initial_remainder = 0,
		const value_type final_xor_value = 0
	) : truncated_polynomial { truncated_polynomial },
		initial_remainder { initial_remainder },
		final_xor_value { final_xor_value },
		remainder { initial_remainder }
	{
	}

	value_type get_initial_remainder() const {
		return initial_remainder;
	}

	void reset(value_type new_initial_remainder) {
		remainder = new_initial_remainder;
	}

	void reset() {
		remainder = initial_remainder;
	}

	void process_bit(bool bit) {
		remainder ^= (bit ? top_bit() : 0U);
		const auto do_poly_div = static_cast<bool>(remainder & top_bit());
		remainder <<= 1;
		if( do_poly_div ) {
			remainder ^= truncated_polynomial;
		}
	}

	void process_bits(value_type bits, size_t bit_count) {
		constexpr auto digits = std::numeric_limits<value_type>::digits;
		constexpr auto mask = static_cast<value_type>(1) << (digits - 1);

		bits <<= (std::numeric_limits<value_type>::digits - bit_count);
		for(size_t i=bit_count; i>0; --i, bits <<= 1) {
			process_bit(static_cast<bool>(bits & mask));
		}
	}

	void process_bits_lsb_first(value_type bits, size_t bit_count) {
		for(size_t i=bit_count; i>0; --i, bits >>= 1) {
			process_bit(static_cast<bool>(bits & 0x01));
		}
	}

	void process_byte(const uint8_t byte) {
		if( RevIn ) {
			process_bits_lsb_first(byte, 8);
		} else {
			process_bits(byte, 8);
		}
	}

	void process_bytes(const uint8_t* const data, const size_t length) {
		for(size_t i=0; i<length; i++) {
			process_byte(data[i]);
		}
	}

	template<size_t N>
	void process_bytes(const std::array<uint8_t, N>& data) {
		process_bytes(data.data(), data.size());
	}

	value_type checksum() const {
		return ((RevOut ? reflect(remainder) : remainder) ^ final_xor_value) & mask();
	}

private:
	const value_type truncated_polynomial;
	const value_type initial_remainder;
	const value_type final_xor_value;
	value_type remainder;

	static constexpr size_t width() {
		return Width;
	}

	static constexpr value_type top_bit() {
		return 1U << (width() - 1);
	}

	static constexpr value_type mask() {
		return (~(~(0UL) << width()));
	}

	static value_type reflect(value_type x) {
		value_type reflection = 0;
		for(size_t i=0; i<width(); ++i) {
			reflection <<= 1;
			reflection |= (x & 1);
			x >>= 1;
		}
		return reflection;
	}
};

#endif/*__CRC_H__*/
