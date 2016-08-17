/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __PROC_ERT_H__
#define __PROC_ERT_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "channel_decimator.hpp"

#include "clock_recovery.hpp"
#include "symbol_coding.hpp"
#include "packet_builder.hpp"
#include "baseband_packet.hpp"

#include "message.hpp"

#include <cstdint>
#include <cstddef>
#include <bitset>

// ''.join(['%d%d' % (c, 1-c) for c in map(int, bin(0x1f2a60)[2:].zfill(21))])
constexpr uint64_t scm_preamble_and_sync_manchester { 0b101010101001011001100110010110100101010101 };
constexpr size_t scm_preamble_and_sync_length { 42 - 10 };
constexpr size_t scm_payload_length_max { 150 };

// ''.join(['%d%d' % (c, 1-c) for c in map(int, bin(0x555516a3)[2:].zfill(32))])
constexpr uint64_t idm_preamble_and_sync_manchester { 0b0110011001100110011001100110011001010110011010011001100101011010 };
constexpr size_t idm_preamble_and_sync_length { 64 - 16 };

constexpr size_t idm_payload_length_max { 1408 };

class ERTProcessor : public BasebandProcessor {
public:
	void execute(const buffer_c8_t& buffer) override;

private:
	const uint32_t baseband_sampling_rate = 4194304;
	const size_t decimation = 1;
	const float symbol_rate = 32768;

	const uint32_t channel_sampling_rate = baseband_sampling_rate / decimation;
	const size_t samples_per_symbol = channel_sampling_rate / symbol_rate;
	const float clock_recovery_rate = symbol_rate * 2;

	BasebandThread baseband_thread { baseband_sampling_rate, this, NORMALPRIO + 20, baseband::Direction::Receive };
	RSSIThread rssi_thread { NORMALPRIO + 10 };

	clock_recovery::ClockRecovery<clock_recovery::FixedErrorFilter> clock_recovery {
		clock_recovery_rate, symbol_rate, { 1.0f / 18.0f },
		[this](const float symbol) { this->consume_symbol(symbol); }
	};

	PacketBuilder<BitPattern, NeverMatch, FixedLength> scm_builder {
		{ scm_preamble_and_sync_manchester, scm_preamble_and_sync_length, 1 },
		{ },
		{ scm_payload_length_max },
		[this](const baseband::Packet& packet) {
			this->scm_handler(packet);
		}
	};

	PacketBuilder<BitPattern, NeverMatch, FixedLength> idm_builder {
		{ idm_preamble_and_sync_manchester, idm_preamble_and_sync_length, 1 },
		{ },
		{ idm_payload_length_max },
		[this](const baseband::Packet& packet) {
			this->idm_handler(packet);
		}
	};

	void consume_symbol(const float symbol);
	void scm_handler(const baseband::Packet& packet);
	void idm_handler(const baseband::Packet& packet);

	float sum_half_period[2];
	float sum_period[3];
	float manchester[3];

	const size_t average_window { 2048 };
	int32_t average_i { 0 };
	int32_t average_q { 0 };
	size_t average_count { 0 };
	float offset_i { 0.0f };
	float offset_q { 0.0f };

	float abs(const complex8_t& v);
};

#endif/*__PROC_ERT_H__*/
