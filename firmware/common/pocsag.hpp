/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __POCSAG_H__
#define __POCSAG_H__

#define POCSAG_PREAMBLE_LENGTH 576
#define POCSAG_TIMEOUT (576 * 2)		// Preamble length * 2
#define POCSAG_SYNCWORD 0x7CD215D8
#define POCSAG_IDLEWORD 0x7A89C197
#define POCSAG_AUDIO_RATE 24000
#define POCSAG_BATCH_LENGTH (17 * 32)

#include "pocsag_packet.hpp"
#include "bch_code.hpp"

namespace pocsag {

// Todo: these enums suck, make a better decode_batch

enum Mode : uint32_t {
	STATE_CLEAR,
	STATE_HAVE_ADDRESS,
	STATE_GETTING_MSG
};

enum OutputType : uint32_t {
	EMPTY,
	ADDRESS,
	MESSAGE
};

enum MessageType : uint32_t {
	ADDRESS_ONLY,
	NUMERIC_ONLY,
	ALPHANUMERIC
};

struct POCSAGState {
	uint32_t function;
	uint32_t address;
	Mode mode = STATE_CLEAR;
	OutputType out_type = EMPTY;
	uint32_t ascii_data;
	uint32_t ascii_idx;
	std::string output;
};

const pocsag::BitRate pocsag_bitrates[3] = {
	pocsag::BitRate::FSK512,
	pocsag::BitRate::FSK1200,
	pocsag::BitRate::FSK2400
};

std::string bitrate_str(BitRate bitrate);
std::string flag_str(PacketFlag packetflag);

void insert_BCH(BCHCode& BCH_code, uint32_t * codeword);
uint32_t get_digit_code(char code);
void pocsag_encode(const MessageType type, BCHCode& BCH_code, const uint32_t function, const std::string message,
					const uint32_t address, std::vector<uint32_t>& codewords);
void pocsag_decode_batch(const POCSAGPacket& batch, POCSAGState * const state);

} /* namespace pocsag */

#endif/*__POCSAG_H__*/
