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

#ifndef __PROC_TPMS_H__
#define __PROC_TPMS_H__

#include "baseband_processor.hpp"

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

// Translate+rectangular filter
// sample=307.2k, deviation=38400, symbol=19200
// Length: 16 taps, 1 symbols, 2 cycles of sinusoid
constexpr std::array<std::complex<float>, 16> rect_taps_307k2_1t_p { {
	{  6.2500000000e-02f,  0.0000000000e+00f }, {  4.4194173824e-02f,  4.4194173824e-02f },
	{  0.0000000000e+00f,  6.2500000000e-02f }, { -4.4194173824e-02f,  4.4194173824e-02f },
	{ -6.2500000000e-02f,  0.0000000000e+00f }, { -4.4194173824e-02f, -4.4194173824e-02f },
	{  0.0000000000e+00f, -6.2500000000e-02f }, {  4.4194173824e-02f, -4.4194173824e-02f },
	{  6.2500000000e-02f,  0.0000000000e+00f }, {  4.4194173824e-02f,  4.4194173824e-02f },
	{  0.0000000000e+00f,  6.2500000000e-02f }, { -4.4194173824e-02f,  4.4194173824e-02f },
	{ -6.2500000000e-02f,  0.0000000000e+00f }, { -4.4194173824e-02f, -4.4194173824e-02f },
	{  0.0000000000e+00f, -6.2500000000e-02f }, {  4.4194173824e-02f, -4.4194173824e-02f },
} };

class TPMSProcessor : public BasebandProcessor {
public:
	TPMSProcessor();

	void execute(const buffer_c8_t& buffer) override;

private:
	dsp::decimate::FIRC8xR16x24FS4Decim4 decim_0;
	dsp::decimate::FIRC16xR16x16Decim2 decim_1;

	dsp::matched_filter::MatchedFilter mf { rect_taps_307k2_1t_p, 8 };

	clock_recovery::ClockRecovery<clock_recovery::FixedErrorFilter> clock_recovery {
		38400, 19200, { 0.0555f },
		[this](const float symbol) { this->consume_symbol(symbol); }
	};
	PacketBuilder<BitPattern, NeverMatch, FixedLength> packet_builder {
		{ 0b010101010101010101010101010110, 30, 1 },
		{ },
		{ 256 },
		[this](const baseband::Packet& packet) {
			this->payload_handler(packet);
		}
	};

	void consume_symbol(const float symbol);
	void payload_handler(const baseband::Packet& packet);
};

#endif/*__PROC_TPMS_H__*/
