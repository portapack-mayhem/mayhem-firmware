/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
 * Copyright (C) 2014 zilog80
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

/* Notes to self (or others, welcome !):
 * Sharebrained wrote in matched_filter.hpp that taps should be those of a complex low-pass filter combined with a complex sinusoid, so
 * that the filter shifts the spectrum where we want (signal of interest around 0Hz).
 * 
 * In this baseband processor, after decim_0 and decim_1, the signal ends up being sampled at 38400Hz (2457600 / 8 / 8)
 * Since the applied shift in ui_sonde.cpp is -fs/4 = -2457600/4 = -614400Hz to avoid the DC spike, the FSK signal ends up being
 * shifted by 614400 / 8 / 8 = 9600Hz. So decim_1_out should look like this:
 * 
 *   _______________|______/'\______
 * -C               A       B       C
 * 
 * A is the DC spike at 0Hz
 * B is the FSK signal shifted right at 9600Hz
 * C is the bandwidth edge at 19200Hz
 * 
 * Taps should be computed to shift the whole spectrum by -9600Hz ("left") so that it looks like this:
 * 
 *   ______________/'\______________
 * -C               D               C
 * 
 * Anything unwanted (like A) should have been filtered off
 * D is B around 0Hz now
 * 
 * Then the clock_recovery function should be happy :)
 * 
 * Mathworks.com says:
 * In the case of a single-rate FIR design, we simply multiply each set of coefficients by (aka 'heterodyne with') a complex exponential.
 * 
 * Can SciPy's remez function be used for this ? See tools/firtest.py
 * GnuRadio's firdes only outputs an odd number of taps
 * 
 * ---------------------------------------------------------------------
 * 
 * Looking at the AIS baseband processor:
 * 
 * Copied everything necessary to get decim_1_out (so same 8 * 8 = 64 decimation factor)
 * The samplerate is also the same (2457600)
 * After the matching filter, the data is decimated by 2 so the final samplerate for clock_recovery is 38400 / 2 = 19200Hz.
 * Like here, the shift used is fs/4, so decim_1_out should be looking similar.
 * The AIS signal deviates by 2400 (4800Hz signal width), the symbol rate is 9600.
 * 
 * The matched filter's input samplerate is 38400Hz, to get a 9600Hz shift it must use 4 taps ?
 * To obtain unity gain, the sinusoid length must be / by the number of taps ?
 * 
 * See ais_baseband.hpp
 * 
 * */

#ifndef __PROC_SONDE_H__
#define __PROC_SONDE_H__

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

class SondeProcessor : public BasebandProcessor {
public:
	SondeProcessor();
	
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

	// Actually 4800bits/s but the Manchester coding doubles the symbol rate
	clock_recovery::ClockRecovery<clock_recovery::FixedErrorFilter> clock_recovery_fsk_9600 {
		19200, 9600, { 0.0555f },
		[this](const float raw_symbol) {
			const uint_fast8_t sliced_symbol = (raw_symbol >= 0.0f) ? 1 : 0;
			this->packet_builder_fsk_9600_Meteomodem.execute(sliced_symbol);
		}
	};
	PacketBuilder<BitPattern, NeverMatch, FixedLength> packet_builder_fsk_9600_Meteomodem {
		{ 0b00110011001100110101100110110011, 32, 1 },
		{ },
		{ 88 * 2 * 8 },
		[this](const baseband::Packet& packet) {
			const SondePacketMessage message { sonde::Packet::Type::Meteomodem_unknown, packet };
			shared_memory.application_queue.push(message);
		}
	};
	
	clock_recovery::ClockRecovery<clock_recovery::FixedErrorFilter> clock_recovery_fsk_4800 {
		19200, 4800, { 0.0555f },
		[this](const float raw_symbol) {
			const uint_fast8_t sliced_symbol = (raw_symbol >= 0.0f) ? 1 : 0;
			this->packet_builder_fsk_4800_Vaisala.execute(sliced_symbol);
		}
	};
	PacketBuilder<BitPattern, NeverMatch, FixedLength> packet_builder_fsk_4800_Vaisala {
		{ 0b00001000011011010101001110001000, 32, 1 }, //euquiq Header detects 4 of 8 bytes 0x10B6CA11 /this is in raw format) (these bits are not passed at the beginning of packet)
		//{ 0b0000100001101101010100111000100001000100011010010100100000011111, 64, 1 }, //euquiq whole header detection would be 8 bytes.
		{ },
		{ 320 * 8 },
		[this](const baseband::Packet& packet) {
			const SondePacketMessage message { sonde::Packet::Type::Vaisala_RS41_SG, packet };
			shared_memory.application_queue.push(message);
		}
	};
};

#endif/*__PROC_ERT_H__*/
