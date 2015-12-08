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

/* Inspired by
 * http://www.barrgroup.com/Embedded-Systems/How-To/CRC-Calculation-C-Code
 *
 * ...then munged into a shape resembling boost::crc_basic.
 * http://www.boost.org/doc/libs/release/libs/crc/
 */

template<typename T>
class CRC {
public:
	constexpr CRC(
		const T truncated_polynomial,
		const T initial_remainder = 0,
		const T final_xor_value = 0
	) : truncated_polynomial { truncated_polynomial },
		initial_remainder { initial_remainder },
		final_xor_value { final_xor_value },
		remainder { initial_remainder }
	{
	}

	T get_initial_remainder() const {
		return initial_remainder;
	}

	void reset(T new_initial_remainder) {
		remainder = new_initial_remainder;
	}

	void reset() {
		remainder = initial_remainder;
	}

	void process_bit(bool bit) {
		remainder ^= bit << (width() - 1);
		if( remainder & top_bit() ) {
			remainder = (remainder << 1) ^ truncated_polynomial;
		} else {
			remainder = (remainder << 1);
		}
	}

	void process_byte(const uint8_t data) {
		remainder ^= data << (width() - 8);
		for(size_t bit=0; bit<8; bit++) {
			if( remainder & top_bit() ) {
				remainder = (remainder << 1) ^ truncated_polynomial;
			} else {
				remainder = (remainder << 1);
			}
		}
	}

	void process_bytes(const uint8_t* const data, const size_t length) {
		for(size_t i=0; i<length; i++) {
			process_byte(data[i]);
		}
	}

	T checksum() const {
		return remainder ^ final_xor_value;
	}

private:
	const T truncated_polynomial;
	const T initial_remainder;
	const T final_xor_value;
	T remainder;

	static constexpr size_t width() {
		return 8 * sizeof(T);
	}

	static constexpr T top_bit() {
		return 1U << (width() - 1);
	}
};


#endif/*__CRC_H__*/
