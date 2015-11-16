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

struct NeverMatch {
	bool operator()(const BitHistory&, const size_t) const {
		return false;
	}
};

struct FixedLength {
	bool operator()(const BitHistory&, const size_t symbols_received) const {
		return symbols_received >= length;
	}

	const size_t length;
};

template<typename PreambleMatcher, typename UnstuffMatcher, typename EndMatcher>
class PacketBuilder {
public:
	using PayloadType = std::bitset<1024>;
	using PayloadHandlerFunc = std::function<void(const PayloadType& payload, const size_t bits_received)>;

	PacketBuilder(
		const PreambleMatcher preamble_matcher,
		const UnstuffMatcher unstuff_matcher,
		const EndMatcher end_matcher,
		const PayloadHandlerFunc payload_handler
	) : payload_handler { payload_handler },
		preamble(preamble_matcher),
		unstuff(unstuff_matcher),
		end(end_matcher)
	{
	}

	void configure(
		const PreambleMatcher preamble_matcher,
		const UnstuffMatcher unstuff_matcher
	) {
		preamble = preamble_matcher;
		unstuff = unstuff_matcher;

		reset_state();
	}

	void execute(
		const uint_fast8_t symbol
	) {
		bit_history.add(symbol);

		switch(state) {
		case State::Preamble:
			if( preamble(bit_history, bits_received) ) {
				state = State::Payload;
			}
			break;

		case State::Payload:
			if( !unstuff(bit_history, bits_received) ) {
				payload[bits_received++] = symbol;
			}

			if( end(bit_history, bits_received) ) {
				payload_handler(payload, bits_received);
				reset_state();
			} else {
				if( packet_truncated() ) {
					reset_state();
				}
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

	const PayloadHandlerFunc payload_handler;

	BitHistory bit_history;
	PreambleMatcher preamble;
	UnstuffMatcher unstuff;
	EndMatcher end;

	size_t bits_received { 0 };
	State state { State::Preamble };
	PayloadType payload;

	void reset_state() {
		bits_received = 0;
		state = State::Preamble;
	}
};

#endif/*__PACKET_BUILDER_H__*/
