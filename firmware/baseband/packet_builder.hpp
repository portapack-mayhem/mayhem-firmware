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
#include "baseband_packet.hpp"

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
	using PayloadHandlerFunc = std::function<void(const baseband::Packet& packet)>;

	PacketBuilder(
		const PreambleMatcher preamble_matcher,
		const UnstuffMatcher unstuff_matcher,
		const EndMatcher end_matcher,
		PayloadHandlerFunc payload_handler
	) : payload_handler { std::move(payload_handler) },
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
			if( preamble(bit_history, packet.size()) ) {
				state = State::Payload;
			}
			break;

		case State::Payload:
			if( !unstuff(bit_history, packet.size()) ) {
				packet.add(symbol);
			}

			if( end(bit_history, packet.size()) ) {
				// NOTE: This check is to avoid std::function nullptr check, which
				// brings in "_ZSt25__throw_bad_function_callv" and a lot of extra code.
				// TODO: Make payload_handler known at compile time.
				if( payload_handler ) {
					packet.set_timestamp(Timestamp::now());
					payload_handler(packet);
				}
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
		return packet.size() >= packet.capacity();
	}

	const PayloadHandlerFunc payload_handler;

	BitHistory bit_history { };
	PreambleMatcher preamble { };
	UnstuffMatcher unstuff { };
	EndMatcher end { };

	State state { State::Preamble };
	baseband::Packet packet { };

	void reset_state() {
		packet.clear();
		state = State::Preamble;
	}
};

#endif/*__PACKET_BUILDER_H__*/
