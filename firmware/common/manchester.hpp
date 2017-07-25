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

#include "baseband_packet.hpp"

struct DecodedSymbol {
	uint_fast8_t value;
	uint_fast8_t error;
};

class ManchesterDecoder {
public:
	constexpr ManchesterDecoder(
		const baseband::Packet& packet,
		const size_t sense = 0
	) : packet { packet },
		sense { sense }
	{
	}

	DecodedSymbol operator[](const size_t index) const;

	size_t symbols_count() const;

private:
	const baseband::Packet& packet;
	const size_t sense;
};

template<typename T>
T operator|(const T& l, const DecodedSymbol& r) {
	return l | r.value;
}

struct FormattedSymbols {
	const std::string data;
	const std::string errors;
};

FormattedSymbols format_symbols(
	const ManchesterDecoder& decoder
);

void manchester_encode(uint8_t * dest, uint8_t * src, size_t length, const size_t sense = 0);

#endif/*__MANCHESTER_H__*/
