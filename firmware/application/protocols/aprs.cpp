/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "aprs.hpp"
#include "ax25.hpp"

#include "portapack_persistent_memory.hpp"

using namespace ax25;

namespace aprs {

void make_aprs_frame(char * address_dest, char * address_src) {
	AX25Frame frame;
	
	char address[14] = { 0 };
	uint8_t info[7] = { 0 };	//{ 'F','U','R','R','T','E','K' };
	
	// Both SSIDs are 0
	memcpy(&address[0], address_dest, 6);
	memcpy(&address[7], address_src, 6);
	
	frame.make_ui_frame(address, 0x03, protocol_id_t::NO_LAYER3, info, sizeof(info));
}

} /* namespace aprs */
