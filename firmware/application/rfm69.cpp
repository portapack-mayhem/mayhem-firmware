/*
 * Copyright (C) 2017 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2019 Furrtek
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

#include "rfm69.hpp"
#include "crc.hpp"
#include "portapack_shared_memory.hpp"

uint32_t RFM69::gen_frame(std::vector<uint8_t>& payload) {
	CRC<16> crc { 0x1021, 0x1D0F, 0xFFFF };
	std::vector<uint8_t> frame { };
	uint8_t byte_out = 0;
	
	// Preamble
	// Really is 0xAA but the RFM69 skips the very last bit (bug ?)
	// so the whole preamble is shifted right to simulate that
	frame.insert(frame.begin(), num_preamble_, 0x55);
	
	// Sync word
	frame.push_back(sync_word_ >> 8);
	frame.push_back(sync_word_ & 0xFF);
	
	// Payload length
	payload.insert(payload.begin(), payload.size());
	
	crc.process_bytes(payload.data(), payload.size());
	
	if (CRC_) {
		payload.push_back(crc.checksum() >> 8);
		payload.push_back(crc.checksum() & 0xFF);
	}
	
	// Manchester-encode payload
	if (manchester_) {
		for (auto byte : payload) {
			for (uint32_t b = 0; b < 8; b++) {
				byte_out <<= 2;
				
				if (byte & 0x80)
					byte_out |= 0b10;
				else
					byte_out |= 0b01;
				
				if ((b & 3) == 3)
					frame.push_back(byte_out);
				
				byte <<= 1;
			}
		}
	} else {
		frame.insert(frame.end(), payload.begin(), payload.end());
	}
	
	memcpy(shared_memory.bb_data.data, frame.data(), frame.size());
	
	// Copy for baseband
	return frame.size();
}
