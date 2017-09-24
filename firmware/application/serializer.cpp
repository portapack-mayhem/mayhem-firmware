/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "serializer.hpp"

#include "portapack_persistent_memory.hpp"

using namespace portapack;

namespace serializer {
	
/* Raw:		00110110100111
 * NRZ-L:	00110110100111
 * NRZ-M:	00100100111010	(1 = transition)
 * NRZ-S:	?0001110010000	(0 = transition)
 * RZ:		00 00 10 10 00 10 10 00 10 00 00 10 10 10	(half bits)
 * Bi-L:	01 01 10 10 01 10 10 01 10 01 01 10 10 10	(1 = high to low, 0 = low to high)
 * Bi-M:	00 11 01 01 00 10 10 11 01 00 11 01 01 01	(1 = mid-bit transition, 0 = invert last level)
 * Bi-S:	01 01 00 11 01 00 11 01 00 10 10 11 00 11	(the opposite)
 * Diff M.:	...
 */

size_t symbol_count(const serial_format_t& serial_format) {
	size_t count;
	
	count = 1 + serial_format.data_bits + serial_format.stop_bits;	// Start + data + stop
	if (serial_format.parity) count++;
	
	return count;
};

} /* namespace serializer */
