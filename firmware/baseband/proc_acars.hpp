/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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

#ifndef __PROC_ACARS_H__
#define __PROC_ACARS_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
//#include "audio_compressor.hpp"

#include "spectrum_collector.hpp"

#include <cstdint>

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

class ACARSProcessor : public BasebandProcessor {
public:
	ACARSProcessor();

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
		19200, 2400, { 0.0555f },
		[this](const float symbol) { this->consume_symbol(symbol); }
	};
	symbol_coding::NRZIDecoder nrzi_decode { };
	PacketBuilder<BitPattern, NeverMatch, BitPattern> packet_builder {
		{ 0b1001011010010110, 16, 1 },	// SYN, SYN
		{ },
		{ 0b11111111, 8, 1 },
		[this](const baseband::Packet& packet) {
			this->payload_handler(packet);
		}
	};

	void consume_symbol(const float symbol);
	void payload_handler(const baseband::Packet& packet);
};

#endif/*__PROC_ACARS_H__*/
