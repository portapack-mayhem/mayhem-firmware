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

#include "rffc507x_spi.hpp"

#include "utility.hpp"

#include "hackrf_gpio.hpp"
using namespace hackrf::one;

namespace rffc507x {
namespace spi {

void SPI::init() {
	gpio_rffc5072_select.set();
	gpio_rffc5072_clock.clear();

	gpio_rffc5072_select.output();
	gpio_rffc5072_clock.output();
	gpio_rffc5072_data.input();

	gpio_rffc5072_data.clear();
}

inline void SPI::select(const bool active) {
	gpio_rffc5072_select.write(!active);
}

inline void SPI::direction_out() {
	gpio_rffc5072_data.output();
}

inline void SPI::direction_in() {
	gpio_rffc5072_data.input();
}

inline void SPI::write_bit(const bit_t value) {
	gpio_rffc5072_data.write(value);
}

inline bit_t SPI::read_bit() {
	return gpio_rffc5072_data.read() & 1;
}

inline bit_t SPI::transfer_bit(const bit_t bit_out) {
	gpio_rffc5072_clock.clear();
	write_bit(bit_out);
	const bit_t bit_in = read_bit();
	gpio_rffc5072_clock.set();
	return bit_in;
}

data_t SPI::transfer_bits(const data_t data_out, const size_t count) {
	data_t data_in = 0;
	for(size_t i=0; i<count; i++) {
		data_in = (data_in << 1) | transfer_bit((data_out >> (count - i - 1)) & 1);
	}
	return data_in;
}

data_t SPI::transfer_word(const Direction direction, const address_t address, const data_t data_out) {
	select(true);

	const data_t address_word =
		  ((direction == Direction::Read) ? (1 << 7) : 0)
		| (address & 0x7f)
		;

	direction_out();
	transfer_bits(address_word, 9);

	if( direction == Direction::Read ) {
		direction_in();
		transfer_bits(0, 2);
	}

	const data_t data_in = transfer_bits(data_out, 16);

	if( direction == Direction::Write ) {
		direction_in();
	}

	select(false);

	transfer_bits(0, 2);

	return data_in;
}

}
}
