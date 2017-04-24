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

#include "lcr.hpp"
#include "string_format.hpp"

namespace lcr {

std::string generate_message(std::string rgsb, std::vector<std::string> litterals, size_t option_ec) {
	const std::string ec_lut[4] = { "A", "J", "N", "S" };					// Eclairage (Auto, Jour, Nuit)
	char eom[3] = { 3, 0, 0 };		// EOM and space for checksum
	uint8_t i;
	std::string lcr_message { 127, 127, 127, 127, 127, 127, 127, 5 };		// 5/15 ? Modem sync and SOM
	char checksum = 0;
	
	// Pad litterals to 7 chars (not required ?)
	for (auto & litteral : litterals)
		while (litteral.length() < 7)
			litteral += ' ';
	
	// Compose LCR message
	lcr_message += rgsb;
	lcr_message += "PA ";
	
	i = 1;
	for (auto & litteral : litterals) {
		lcr_message += "AM=";
		lcr_message += to_string_dec_uint(i, 1);
		lcr_message += " AF=\"";
		lcr_message += litteral;
		lcr_message += "\" CL=0 ";
		i++;
	}
	
	lcr_message += "EC=";
	lcr_message += ec_lut[option_ec];
	lcr_message += " SAB=0";
	
	// Checksum
	i = 7;						// Skip modem sync
	while (lcr_message[i])
		checksum ^= lcr_message[i++];
	checksum ^= eom[0];			// EOM char
	checksum &= 0x7F;			// Trim
	eom[1] = checksum;
	
	lcr_message += eom;
	
	return lcr_message;
}

} /* namespace lcr */
