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

#ifndef __EMU_CC1101_H__
#define __EMU_CC1101_H__

#include <cstdint>
#include <array>

#include "utility.hpp"

namespace cc1101 {
	
// Data rate (Bauds)
// Whitening: Everything except preamble and sync word, init value = 111111111
// Packet format: preamble, sync word, (opt) length, (opt) address, payload, (opt) CRC
// Preamble: 8*n bits of 10101010
// Sync word: 2 bytes (can be repeated twice)
// Length: 1 byte (address + payload)
// 2-FSK: 0=-dev, 1=+dev
// 4-FSK: 00=-1/3dev, 01=-dev, 10=1/3dev, 11=+dev (preamble and sync are in 2-FSK)
// OOK: PA on or off
// ASK: Power can be adjusted
// FEC: ?

class CC1101Emu {
public:
	//CC1101Emu();
	//~CC1101Emu();
	
	enum packet_mode_t {
		FIXED_LENGTH,
		VARIABLE_LENGTH,
		INFINITE_LENGTH
	};
	
	enum modulation_t {
		TWO_FSK,
		GFSK,
		OOK,
		FOUR_FSK,
		MSK,
	};
	
	void set_sync_word(const uint16_t sync_word) {
		sync_word_ = sync_word;
	};
	void set_address(const uint8_t address) {
		address_ = address;
	};
	void set_packet_length(const uint8_t packet_length) {
		packet_length_ = packet_length;
	};
	void set_data_config(const bool CRC, const bool manchester, const bool whitening) {
		CRC_ = CRC;
		manchester_ = manchester;
		whitening_ = whitening;
	};
	void set_packet_mode(const packet_mode_t packet_mode) {
		packet_mode_ = packet_mode;
	};
	void set_modulation(const modulation_t modulation) {
		modulation_ = modulation;
	}
	void set_num_preamble(const uint8_t num_preamble) {		// 2, 3, 4, 6, 8, 12, 16, or 24
		num_preamble_ = num_preamble;
	};
	void set_deviation(const size_t deviation) {
		deviation_ = deviation;
	};

private:
	uint16_t sync_word_ { 0xD391 };
	uint8_t address_ { 0x00 };
	uint8_t packet_length_ { 0 };
	bool CRC_ { false };
	bool manchester_ { false };
	bool whitening_ { true };
	packet_mode_t packet_mode_ { VARIABLE_LENGTH };
	modulation_t modulation_ { TWO_FSK };
	uint8_t num_preamble_ { 4 };
	size_t deviation_ { 4000 };
	
	uint16_t whitening_pn { 0x1FF };
	
	void whitening_init();
	uint8_t whiten_byte(uint8_t byte);
	
};

} /* namespace cc1101 */

#endif/*__EMU_CC1101_H__*/
