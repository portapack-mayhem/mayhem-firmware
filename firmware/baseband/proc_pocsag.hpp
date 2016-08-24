/*
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

#include "dsp_iir.hpp"
#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"

#include "pocsag_packet.hpp"

#include "message.hpp"
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
		LOSING_SYNC = 65,
		LOST_SYNC = 66,
		ADDRESS = 67,
		MESSAGE = 68,
		END_OF_MESSAGE = 69
	};

	static constexpr size_t baseband_fs = 1536000;

	BasebandThread baseband_thread { baseband_fs, this, NORMALPRIO + 20, baseband::Direction::Receive };
	RSSIThread rssi_thread { NORMALPRIO + 10 };
	
	std::array<complex16_t, 512> dst;
	const buffer_c16_t dst_buffer {
		dst.data(),
		dst.size()
	};
	std::array<float, 32> audio;
	const buffer_f32_t audio_buffer {
		audio.data(),
		audio.size()
	};

	dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0;
	dsp::decimate::FIRC16xR16x32Decim8 decim_1;

	dsp::demodulate::FM demod;

	uint32_t sync_timeout;
	uint32_t msg_timeout;

	uint32_t dcd_shreg;
	uint32_t sphase;
	uint32_t rx_data;
	uint32_t rx_bit;
	bool configured = false;
	rx_states rx_state;
	
	size_t c, frame_counter;
	
	pocsag::POCSAGPacket packet;
	
	void configure(const POCSAGConfigureMessage& message);
	
};

#endif/*__PROC_POCSAG_H__*/
