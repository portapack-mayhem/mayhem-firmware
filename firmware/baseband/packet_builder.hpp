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

#ifndef __PACKET_BUILDER_H__
#define __PACKET_BUILDER_H__

#include <cstdint>
#include <cstddef>
#include <bitset>

#include "symbol_coding.hpp"

class PacketBuilder {
public:
	void configure(
		uint32_t unstuffing_pattern,
		size_t unstuffing_length,
		size_t new_payload_length
	);

	template<typename PayloadHandler>
	void execute(
		const uint_fast8_t symbol,
		const bool access_code_found,
		PayloadHandler payload_handler
	) {
		switch(state) {
		case State::AccessCodeSearch:
			if( access_code_found ) {
				state = State::Payload;
			}
			break;

		case State::Payload:
			if( bits_received < payload_length ) {
				if( !unstuff.is_stuffing_bit(symbol) ) {
					payload[bits_received++] = symbol;
				}
			} else {
				payload_handler(payload, bits_received);
				reset_state();
			}
			break;

		default:
			reset_state();
			break;
		}
	}

private:
	enum State {
		AccessCodeSearch,
		Payload,
	};

	size_t payload_length { 0 };
	size_t bits_received { 0 };
	State state { State::AccessCodeSearch };
	std::bitset<256> payload;
	symbol_coding::Unstuff unstuff;

	void reset_state();
};

#endif/*__PACKET_BUILDER_H__*/
