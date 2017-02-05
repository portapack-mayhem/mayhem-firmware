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

#define POCSAG_TIMEOUT (576 * 2)	// Preamble length * 2
#define POCSAG_SYNC 0x7CD215D8
#define POCSAG_IDLE 0x7A89C197
#define POCSAG_AUDIO_RATE 24000
#define POCSAG_BATCH_LENGTH (17 * 32)

#include "pocsag_packet.hpp"

namespace pocsag {

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

struct POCSAGState {
	uint32_t function;
	uint32_t address;
	Mode mode = STATE_CLEAR;
	OutputType out_type = EMPTY;
	uint32_t ascii_data;
	uint32_t ascii_idx;
	std::string output;
};

std::string bitrate_str(BitRate bitrate);
std::string flag_str(PacketFlag packetflag);

bool decode_batch(const POCSAGPacket& batch, POCSAGState * const state);

} /* namespace pocsag */

#endif/*__POCSAG_H__*/
