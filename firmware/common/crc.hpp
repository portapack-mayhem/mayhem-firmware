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

template<typename T>
class CRC {
public:
	constexpr CRC(
		const T polynomial/*,
		const T initial*/
	) : polynomial { polynomial }/*,
		initial { initial }*/
	{
		// CRC LSB must always be 1
	}
/*
	template<typename U>
	T calculate(const U& bits, const size_t length) {
		if( length > bits.size() ) {
			// Exception.
			return 0;
		}

		T crc = 0;

		for(size_t i=0; i<length; i++) {
			crc = feed_bit(crc, bits[i]);
		}

		return crc;
	}
*/
	T calculate_byte(const T crc_in, const uint8_t data) {
		/* Inspired by
		 * http://www.barrgroup.com/Embedded-Systems/How-To/CRC-Calculation-C-Code
		 */
		T remainder = crc_in;
		remainder ^= data << (width() - 8);
		for(size_t bit=8; bit>0; --bit) {
			if( remainder & top_bit() ) {
				remainder = (remainder << 1) ^ polynomial;
			} else {
				remainder = (remainder << 1);
			}
		}
		return remainder;
	}
/*
	T calculate(const uint8_t* const data, const size_t length) {
		T remainder = initial;
		for(size_t byte=0; byte<length; ++byte) {
			remainder = calculate_byte(remainder, data[byte]);
		}
		return remainder;
	}
*/
private:
	const T polynomial;
//	const T initial;

	static constexpr size_t width() {
		return 8 * sizeof(T);
	}

	static constexpr T top_bit() {
		return 1U << (width() - 1);
	}
/*
	T feed_bit(const T crc_in, const uint_fast8_t bit) {
		T crc_out = crc_in ^ (bit & 1);
		if( crc_in & top_bit() ) {
			return ((crc_in << 1) | (bit & 1)) ^ polynomial;
		} else {
			return (crc_in << 1) | (bit & 1);
		}
	}
*/
};


#endif/*__CRC_H__*/
