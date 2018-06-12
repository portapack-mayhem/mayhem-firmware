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

// AIS:
// IN: 2457600/8/8 = 38400
// Offset: 2457600/4 = 614400 (614400/8/8 = 9600)
// Deviation: 2400
// Symbol: 9600
// Decimate: 2
// 4 taps, 1 symbol, 1/4 cycle

// TPMS:
// IN: 2457600/4/2 = 307200
// Offset: 2457600/4 = 614400 (614400/4/2 = 76800)
// Deviation: 38400
// Symbol: 19200
// Decimate: 8
// 16 taps, 1 symbol, 2 cycles

// ACARS:
// IN: 2457600/8/8 = 38400
// Offset: 2457600/4 = 614400 (614400/8/8 = 9600)
// Deviation: ???
// Symbol: 2400
// Decimate: 8
// 16 taps, 1 symbol, 2 cycles

// Number of taps: size of one symbol in samples (in/symbol)
// Cycles: 


// Translate+rectangular filter
// sample=38.4k, deviation=4800, symbol=2400
// Length: 16 taps, 1 symbol, 2 cycles of sinusoid
// This is actually the same as rect_taps_307k2_38k4_1t_19k2_p
constexpr std::array<std::complex<float>, 16> rect_taps_38k4_4k8_1t_2k4_p { {
	{  6.2500000000e-02f,  0.0000000000e+00f }, {  4.4194173824e-02f,  4.4194173824e-02f },
	{  0.0000000000e+00f,  6.2500000000e-02f }, { -4.4194173824e-02f,  4.4194173824e-02f },
	{ -6.2500000000e-02f,  0.0000000000e+00f }, { -4.4194173824e-02f, -4.4194173824e-02f },
	{  0.0000000000e+00f, -6.2500000000e-02f }, {  4.4194173824e-02f, -4.4194173824e-02f },
	{  6.2500000000e-02f,  0.0000000000e+00f }, {  4.4194173824e-02f,  4.4194173824e-02f },
	{  0.0000000000e+00f,  6.2500000000e-02f }, { -4.4194173824e-02f,  4.4194173824e-02f },
	{ -6.2500000000e-02f,  0.0000000000e+00f }, { -4.4194173824e-02f, -4.4194173824e-02f },
	{  0.0000000000e+00f, -6.2500000000e-02f }, {  4.4194173824e-02f, -4.4194173824e-02f },
} };

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

	dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0 { };	// Translate already done here !
	dsp::decimate::FIRC16xR16x32Decim8 decim_1 { };
	dsp::matched_filter::MatchedFilter mf { rect_taps_38k4_4k8_1t_2k4_p, 8 };

	clock_recovery::ClockRecovery<clock_recovery::FixedErrorFilter> clock_recovery {
		4800, 2400, { 0.0555f },
		[this](const float symbol) { this->consume_symbol(symbol); }
	};
	symbol_coding::ACARSDecoder acars_decode { };
	/*PacketBuilder<BitPattern, NeverMatch, FixedLength> packet_builder {
		{ 0b011010000110100010000000, 24, 1 },	// SYN, SYN, SOH
		{ },
		{ 128 },
		[this](const baseband::Packet& packet) {
			this->payload_handler(packet);
		}
	};*/
	baseband::Packet packet { };

	void consume_symbol(const float symbol);
	void payload_handler(const baseband::Packet& packet);
};

#endif/*__PROC_ACARS_H__*/
