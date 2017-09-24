/*
 * Copyright (C) 2017 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#include "emu_cc1101.hpp"

namespace cc1101 {

void CC1101Emu::whitening_init() {
	whitening_pn = 0x1FF;
}

// See TI app note DN509
uint8_t CC1101Emu::whiten_byte(uint8_t byte) {
	uint_fast8_t new_bit;
	
	byte ^= (whitening_pn & 0xFF);
	
	for (size_t step = 0; step < 8; step++) {
		new_bit = (whitening_pn & 1) ^ ((whitening_pn >> 5) & 1);
		whitening_pn >>= 1;
		whitening_pn |= (new_bit << 8);
	}
	
	return byte;
}

} /* namespace cc1101 */
