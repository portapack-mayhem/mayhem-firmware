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
#include <functional>

#include "bit_pattern.hpp"

class PacketBuilder {
public:
	using PayloadType = std::bitset<1024>;
	using PayloadHandlerFunc = std::function<void(const PayloadType& payload, const size_t bits_received)>;

	PacketBuilder(
		const PayloadHandlerFunc payload_handler
	) : payload_handler { payload_handler }
	{
	}

	void configure(
		const BitPattern preamble,
		const BitPattern unstuffing
	);

	void execute(
		const uint_fast8_t symbol
	) {
		bit_history.add(symbol);

		switch(state) {
		case State::Preamble:
			if( found_preamble() ) {
				state = State::Payload;
			}
			break;

		case State::Payload:
			if( !found_stuffing_bit() ) {
				payload[bits_received++] = symbol;
			}

			if( found_end_flag() || packet_truncated() ) {
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
		Preamble,
		Payload,
	};

	bool packet_truncated() const {
		return bits_received >= payload.size();
	}

	bool found_preamble() const {
		return bit_history.matches(preamble_pattern);
	}

	bool found_stuffing_bit() const {
		return bit_history.matches(unstuff_pattern);
	}

	bool found_end_flag() const {
		return bit_history.matches(end_flag_pattern);
	}

	const PayloadHandlerFunc payload_handler;

	BitHistory bit_history;
	BitPattern preamble_pattern { 0b01010101010101010101111110, 26, 1 };
	BitPattern unstuff_pattern { 0b111110, 6 };
	BitPattern end_flag_pattern { 0b01111110, 8 };

	size_t bits_received { 0 };
	State state { State::Preamble };
	PayloadType payload;

	void reset_state();
};

#endif/*__PACKET_BUILDER_H__*/
