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

#ifndef __MANCHESTER_H__
#define __MANCHESTER_H__

#include <cstdint>
#include <cstddef>
#include <bitset>
#include <string>

class ManchesterDecoder {
public:
	struct DecodedSymbol {
		uint_fast8_t value;
		uint_fast8_t error;
	};

	constexpr ManchesterDecoder(
		const std::bitset<1024>& encoded,
		const size_t count,
		const size_t sense = 0
	) : encoded { encoded },
		count { count },
		sense { sense }
	{
	}

	DecodedSymbol operator[](const size_t index) const {
		const size_t encoded_index = index * 2;
		if( (encoded_index + 1) < count ) {
			const auto value = encoded[encoded_index + sense];
			const auto error = encoded[encoded_index + 0] == encoded[encoded_index + 1];
			return { value, error };
		} else {
			return { 0, 1 };
		}
	}

	size_t symbols_count() const {
		return count / 2;
	}

private:
	const std::bitset<1024>& encoded;
	const size_t count;
	const size_t sense;
};

struct ManchesterFormatted {
	const std::string data;
	const std::string errors;
};

ManchesterFormatted format_manchester(
	const ManchesterDecoder& decoder
);

#endif/*__MANCHESTER_H__*/
