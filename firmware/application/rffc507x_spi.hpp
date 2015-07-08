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

#ifndef __RFFC507X_SPI_H__
#define __RFFC507X_SPI_H__

#include <cstdint>
#include <cstddef>

namespace rffc507x {
namespace spi {

using reg_t = uint16_t;
using address_t = uint8_t;
using bit_t = uint_fast8_t;
using data_t = uint32_t;

class SPI {
public:
	enum class Direction {
		Write = 0,
		Read = 1,
	};

	void init();

	reg_t read(const address_t address) {
		return transfer_word(Direction::Read, address, 0);
	}

	void write(const address_t address, const reg_t value) {
		transfer_word(Direction::Write, address, value);
	}

private:
	void select(const bool active);

	void direction_out();
	void direction_in();

	void write_bit(const bit_t value);
	bit_t read_bit();

	bit_t transfer_bit(const bit_t bit_out);
	data_t transfer_bits(const data_t data_out, const size_t count);
	data_t transfer_word(const Direction direction, const address_t address, const data_t data_out);
};

} /* spi */
} /* rffc507x */

#endif/*__RFFC507X_SPI_H__*/
