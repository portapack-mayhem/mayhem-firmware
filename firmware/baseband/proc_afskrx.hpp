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

#ifndef __PROC_AFSKRX_H__
#define __PROC_AFSKRX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"

#include "audio_output.hpp"

#include "fifo.hpp"
#include "message.hpp"

class AFSKRxProcessor : public BasebandProcessor {
public:
	void execute(const buffer_c8_t& buffer) override;

	void on_message(const Message* const message) override;
	
private:
	static constexpr size_t baseband_fs = 3072000;
	static constexpr size_t audio_fs = baseband_fs / 8 / 8 / 2;
	
	size_t samples_per_bit { };
	
	enum State {
		WAIT_START = 0,
		WAIT_STOP,
		RECEIVE
	};
	
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
	
	// Array size ok down to 375 bauds (24000 / 375)
	std::array<int32_t, 64> delay_line { 0 };
	
	dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0 { };
	dsp::decimate::FIRC16xR16x32Decim8 decim_1 { };
	dsp::decimate::FIRAndDecimateComplex channel_filter { };
	
	dsp::demodulate::FM demod { };
	
	AudioOutput audio_output { };

	State state { };
	size_t delay_line_index { };
	uint32_t bit_counter { 0 };
	uint32_t word_bits { 0 };
	uint32_t sample_bits { 0 };
	uint32_t phase { }, phase_inc { };
	int32_t sample_mixed { }, prev_mixed { }, sample_filtered { }, prev_filtered { };
	uint32_t word_length { };
	uint32_t word_mask { };
	uint32_t trigger_value { };
	
	bool configured { false };
	bool wait_start { };
	bool bit_value { };
	bool trigger_word { };
	bool triggered { };
	
	void configure(const AFSKRxConfigureMessage& message);
	
	AFSKDataMessage data_message { false, 0 };
};

#endif/*__PROC_TPMS_H__*/
