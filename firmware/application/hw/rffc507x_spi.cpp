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

#include "gpio.hpp"
using namespace gpio_control;

namespace rffc507x {
namespace spi {

void SPI::init() {
    rffc5072_select.setInactive();
    rffc5072_clock.setInactive();
    rffc5072_sdata.input();
    rffc5072_sdata.setInactive();
}

inline void SPI::select(const bool active) {
    rffc5072_select.setState(active);
}

inline void SPI::direction_out() {
    rffc5072_sdata.output();
}

inline void SPI::direction_in() {
    rffc5072_sdata.input();
}

inline void SPI::write_bit(const bit_t value) {
    rffc5072_sdata.write(value);
}

inline bit_t SPI::read_bit() {
    return rffc5072_sdata.read() & 1;
}

inline bit_t SPI::transfer_bit(const bit_t bit_out) {
    rffc5072_clock.setInactive();
    write_bit(bit_out);
    const bit_t bit_in = read_bit();
    rffc5072_clock.setActive();
    return bit_in;
}

data_t SPI::transfer_bits(const data_t data_out, const size_t count) {
    data_t data_in = 0;
    for (size_t i = 0; i < count; i++) {
        data_in = (data_in << 1) | transfer_bit((data_out >> (count - i - 1)) & 1);
    }
    return data_in;
}

data_t SPI::transfer_word(const Direction direction, const address_t address, const data_t data_out) {
    select(true);

    const data_t address_word =
        ((direction == Direction::Read) ? (1 << 7) : 0) | (address & 0x7f);

    direction_out();
    transfer_bits(address_word, 9);

    if (direction == Direction::Read) {
        direction_in();
        transfer_bits(0, 2);
    }

    const data_t data_in = transfer_bits(data_out, 16);

    if (direction == Direction::Write) {
        direction_in();
    }

    select(false);

    transfer_bits(0, 2);

    return data_in;
}

void SPI::power_down() {
    // A 0x01 address the MIX_CTRL register. A 0x0000
    transfer_word(Direction::Write, 0x01, 0x0000);

    // a chip 300 µA-es Power Down
    transfer_word(Direction::Write, 0x00, 0x0000);
}

}  // namespace spi
}  // namespace rffc507x
