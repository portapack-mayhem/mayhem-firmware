/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2020 Shao
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

#ifndef __PROC_BTLERX_H__
#define __PROC_BTLERX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"

#include "audio_output.hpp"

#include "fifo.hpp"
#include "message.hpp"

class BTLERxProcessor : public BasebandProcessor {
public:
	void execute(const buffer_c8_t& buffer) override;

	void on_message(const Message* const message) override;
	
private:
	static constexpr size_t baseband_fs = 4000000;
	static constexpr size_t audio_fs = baseband_fs / 8 / 8 / 2;
	
	BasebandThread baseband_thread { baseband_fs, this, NORMALPRIO + 20, baseband::Direction::Receive };
	RSSIThread rssi_thread { NORMALPRIO + 10 };
	
	std::array<complex16_t, 512> dst { };
	const buffer_c16_t dst_buffer {
		dst.data(),
		dst.size()
	};

        std::array<complex16_t, 512> spectrum { };
	const buffer_c16_t spectrum_buffer {
		spectrum.data(),
		spectrum.size()
	};

	const buffer_s16_t work_audio_buffer {
		(int16_t*)dst.data(),
		sizeof(dst) / sizeof(int16_t)
	};


	// Array size ok down to 375 bauds (24000 / 375)
	std::array<int32_t, 64> delay_line { 0 };
	std::array<int16_t, 1000> rb_buf { 0 };

	/*dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0 { };
	dsp::decimate::FIRC16xR16x32Decim8 decim_1 { };
	dsp::decimate::FIRAndDecimateComplex channel_filter { };*/
	dsp::decimate::FIRC8xR16x24FS4Decim4 decim_0 { };
	dsp::decimate::FIRC16xR16x16Decim2 decim_1 { };
	
	dsp::demodulate::FM demod { };
	int rb_head {-1};
	int32_t g_threshold {0};  
	uint8_t channel_number {38};
	int skipSamples {1000};
	int RB_SIZE {1000};

	bool configured { false };

	
	void configure(const BTLERxConfigureMessage& message);
	
	AFSKDataMessage data_message { false, 0 };
};

#endif/*__PROC_BTLERX_H__*/
