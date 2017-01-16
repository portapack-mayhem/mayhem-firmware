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
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "channel_decimator.hpp"
#include "matched_filter.hpp"

#include "clock_recovery.hpp"
#include "symbol_coding.hpp"
#include "packet_builder.hpp"
#include "baseband_packet.hpp"

#include "ook.hpp"

#include "message.hpp"
#include "portapack_shared_memory.hpp"

#include <cstdint>
#include <cstddef>
#include <bitset>

// Translate+rectangular filter
// sample=307.2k, deviation=38400, symbol=19200
// Length: 16 taps, 1 symbols, 2 cycles of sinusoid
constexpr std::array<std::complex<float>, 16> rect_taps_307k2_38k4_1t_19k2_p { {
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
	static constexpr size_t baseband_fs = 2457600;

	BasebandThread baseband_thread { baseband_fs, this, NORMALPRIO + 20, baseband::Direction::Receive };
	RSSIThread rssi_thread { NORMALPRIO + 10 };

	std::array<complex16_t, 512> dst { };
	const buffer_c16_t dst_buffer {
		dst.data(),
		dst.size()
	};

	dsp::decimate::FIRC8xR16x24FS4Decim4 decim_0 { };
	dsp::decimate::FIRC16xR16x16Decim2 decim_1 { };

	dsp::matched_filter::MatchedFilter mf_38k4_1t_19k2 { rect_taps_307k2_38k4_1t_19k2_p, 8 };

	clock_recovery::ClockRecovery<clock_recovery::FixedErrorFilter> clock_recovery_fsk_19k2 {
		38400, 19200, { 0.0555f },
		[this](const float raw_symbol) {
			const uint_fast8_t sliced_symbol = (raw_symbol >= 0.0f) ? 1 : 0;
			this->packet_builder_fsk_19k2_schrader.execute(sliced_symbol);
		}
	};
	PacketBuilder<BitPattern, NeverMatch, FixedLength> packet_builder_fsk_19k2_schrader {
		{ 0b010101010101010101010101010110, 30, 1 },
		{ },
		{ 160 },
		[this](const baseband::Packet& packet) {
			const TPMSPacketMessage message { tpms::SignalType::FSK_19k2_Schrader, packet };
			shared_memory.application_queue.push(message);
		}
	};

	static constexpr float channel_rate_in = 307200.0f;
	static constexpr size_t channel_decimation = 2;
	static constexpr float channel_sample_rate = channel_rate_in / channel_decimation;
	OOKSlicerMagSquaredInt ook_slicer_5sps { channel_sample_rate / 8400 + 1};
	uint32_t slicer_history { 0 };

	OOKClockRecovery clock_recovery_ook_8k192 {
		channel_sample_rate / 8192.0f
	};

	PacketBuilder<BitPattern, NeverMatch, FixedLength> packet_builder_ook_8k192_schrader {
		/* Preamble: 11*2, 01*14, 11, 10
		 * Payload: 37 Manchester-encoded bits
		 * Bit rate: 4096 Hz
		 */
		{ 0b010101010101010101011110, 24, 0 },
		{ },
		{ 37 * 2 },
		[](const baseband::Packet& packet) {
			const TPMSPacketMessage message { tpms::SignalType::OOK_8k192_Schrader, packet };
			shared_memory.application_queue.push(message);
		}
	};

	OOKClockRecovery clock_recovery_ook_8k4 {
		channel_sample_rate / 8400.0f
	};

	PacketBuilder<BitPattern, NeverMatch, FixedLength> packet_builder_ook_8k4_schrader {
		/* Preamble: 01*40, 01, 10, 01, 01
		 * Payload: 76 Manchester-encoded bits
		 * Bit rate: 4200 Hz
		 */
		{ 0b01010101010101010101010101100101, 32, 0 },
		{ },
		{ 76 * 2 },
		[](const baseband::Packet& packet) {
			const TPMSPacketMessage message { tpms::SignalType::OOK_8k4_Schrader, packet };
			shared_memory.application_queue.push(message);
		}
	};
};

#endif/*__PROC_TPMS_H__*/
