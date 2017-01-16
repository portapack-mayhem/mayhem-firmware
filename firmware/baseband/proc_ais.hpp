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

#ifndef __PROC_AIS_H__
#define __PROC_AIS_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "channel_decimator.hpp"
#include "matched_filter.hpp"

#include "clock_recovery.hpp"
#include "symbol_coding.hpp"
#include "packet_builder.hpp"
#include "baseband_packet.hpp"

#include "message.hpp"

#include <cstdint>
#include <cstddef>
#include <bitset>

#include "ais_baseband.hpp"

class AISProcessor : public BasebandProcessor {
public:
	AISProcessor();

	void execute(const buffer_c8_t& buffer) override;

private:
	static constexpr size_t baseband_fs = 2457600;

	BasebandThread baseband_thread { baseband_fs, this, NORMALPRIO + 20, baseband::Direction::Receive };
	RSSIThread rssi_thread { NORMALPRIO + 10 };

	std::array<complex16_t, 512> dst { };
	const buffer_c16_t dst_buffer {
		dst.data(),
		dst.size()
	};

	dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0 { };
	dsp::decimate::FIRC16xR16x32Decim8 decim_1 { };
	dsp::matched_filter::MatchedFilter mf { baseband::ais::square_taps_38k4_1t_p, 2 };

	clock_recovery::ClockRecovery<clock_recovery::FixedErrorFilter> clock_recovery {
		19200, 9600, { 0.0555f },
		[this](const float symbol) { this->consume_symbol(symbol); }
	};
	symbol_coding::NRZIDecoder nrzi_decode { };
	PacketBuilder<BitPattern, BitPattern, BitPattern> packet_builder {
		{ 0b0101010101111110, 16, 1 },
		{ 0b111110, 6 },
		{ 0b01111110, 8 },
		[this](const baseband::Packet& packet) {
			this->payload_handler(packet);
		}
	};

	void consume_symbol(const float symbol);
	void payload_handler(const baseband::Packet& packet);
};

#endif/*__PROC_AIS_H__*/
