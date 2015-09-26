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

#include "access_code_correlator.hpp"

void AccessCodeCorrelator::configure(
	const uint32_t new_code,
	const size_t new_code_length,
	const size_t new_maximum_hamming_distance
) {
	if( new_code_length <= 32 ) {
		code = new_code;
		mask = mask_value(new_code_length);
		maximum_hamming_distance = new_maximum_hamming_distance;
	}
}
