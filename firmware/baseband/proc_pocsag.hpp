/*
 * Copyright (C) 1996 Thomas Sailer (sailer@ife.ee.ethz.ch, hb9jnx@hb9w.che.eu)
 * Copyright (C) 2012-2014 Elias Oenal (multimon-ng@eliasoenal.com)
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#ifndef __PROC_POCSAG_H__
#define __PROC_POCSAG_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"

#include "pocsag_packet.hpp"

#include "pocsag.hpp"
#include "message.hpp"
#include "audio_output.hpp"
#include "portapack_shared_memory.hpp"

#include <cstdint>

class POCSAGProcessor : public BasebandProcessor {
public:
	void execute(const buffer_c8_t& buffer) override;
	
	void on_message(const Message* const message) override;

private:
	enum rx_states {
		WAITING = 0,
		PREAMBLE = 32,
		SYNC = 64,
		//LOSING_SYNC = 65,
		//LOST_SYNC = 66,
		//ADDRESS = 67,
		//MESSAGE = 68,
		//END_OF_MESSAGE = 69
	};

	static constexpr size_t baseband_fs = 3072000;

	BasebandThread baseband_thread { baseband_fs, this, NORMALPRIO + 20, baseband::Direction::Receive };
	RSSIThread rssi_thread { NORMALPRIO + 10 };
	
	std::array<complex16_t, 512> dst { };
	const buffer_c16_t dst_buffer {
		dst.data(),
		dst.size()
	};
	std::array<float, 32> audio { };
	const buffer_f32_t audio_buffer {
		audio.data(),
		audio.size()
	};

	dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0 { };
	dsp::decimate::FIRC16xR16x32Decim8 decim_1 { };
	dsp::decimate::FIRAndDecimateComplex channel_filter { };
	dsp::demodulate::FM demod { };
	
	//AudioOutput audio_output { };

	uint32_t sync_timeout { 0 };
	uint32_t msg_timeout { 0 };

	uint32_t slicer_sr { 0 };
	uint32_t sphase { 0 };
	uint32_t sphase_delta { 0 };
	uint32_t sphase_delta_half { 0 };
	uint32_t sphase_delta_eighth { 0 };
	uint32_t rx_data { 0 };
	uint32_t rx_bit { 0 };
	bool configured = false;
	rx_states rx_state { WAITING };
	pocsag::BitRate bitrate { pocsag::BitRate::FSK1200 };
	bool phase;
	uint32_t codeword_count { 0 };
	pocsag::POCSAGPacket packet { };
	
	void push_packet(pocsag::PacketFlag flag);
	void configure(const POCSAGConfigureMessage& message);
	
};

#endif/*__PROC_POCSAG_H__*/
