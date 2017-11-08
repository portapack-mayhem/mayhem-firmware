/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __PROC_TEST_H__
#define __PROC_TEST_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"
#include "proc_ais.hpp"

#include "channel_decimator.hpp"
#include "matched_filter.hpp"

#include "clock_recovery.hpp"
#include "symbol_coding.hpp"
#include "packet_builder.hpp"
#include "baseband_packet.hpp"

#include "message.hpp"
#include "portapack_shared_memory.hpp"

#include <cstdint>
#include <cstddef>
#include <bitset>

class TestProcessor : public BasebandProcessor {
public:
	TestProcessor();
	
	void execute(const buffer_c8_t& buffer) override;

private:
	static constexpr size_t baseband_fs = 2457600*2;
	
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

	clock_recovery::ClockRecovery<clock_recovery::FixedErrorFilter> clock_recovery_fsk_9600 {
		38400, 19192, { 0.00555f },
		[this](const float raw_symbol) {
			const uint_fast8_t sliced_symbol = (raw_symbol >= 0.0f) ? 1 : 0;
			this->packet_builder_fsk_9600_CC1101.execute(sliced_symbol);
		}
	};
	PacketBuilder<BitPattern, NeverMatch, FixedLength> packet_builder_fsk_9600_CC1101 {
		{ 0b01010110010110100101101001101010, 32, 1 },	// Manchester 0x1337
		{ },
		{ 22 * 8 },
		[this](const baseband::Packet& packet) {
			const TestAppPacketMessage message { packet };
			shared_memory.application_queue.push(message);
		}
	};
};

#endif/*__PROC_TEST_H__*/
