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

#ifndef __SYMBOL_CODING_H__
#define __SYMBOL_CODING_H__

#include <cstdint>
#include <cstddef>

namespace symbol_coding {

class NRZIDecoder {
public:
	uint_fast8_t operator()(const uint_fast8_t symbol) {
		const auto out = (~(symbol ^ last)) & 1;
		last = symbol;
		return out;
	}

private:
	uint_fast8_t last { 0 };
};

class ACARSDecoder {
public:
	uint_fast8_t operator()(const uint_fast8_t symbol) {
		last ^= (~symbol & 1);
		return last;
	}

private:
	uint_fast8_t last { 0 };
};

} /* namespace symbol_coding */

#endif/*__SYMBOL_CODING_H__*/
