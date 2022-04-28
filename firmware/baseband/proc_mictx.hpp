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

#ifndef __PROC_MICTX_H__
#define __PROC_MICTX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "audio_input.hpp"
#include "tone_gen.hpp"
#include "dsp_modulate.hpp"

class MicTXProcessor : public BasebandProcessor {
public:
	void execute(const buffer_c8_t& buffer) override;
	
	void on_message(const Message* const msg) override;

private:
	static constexpr size_t baseband_fs = 1536000U;
	
	bool configured { false };
	
	BasebandThread baseband_thread { baseband_fs, this, NORMALPRIO + 20, baseband::Direction::Transmit };
	
	int16_t audio_data[64];
	buffer_s16_t audio_buffer {
		audio_data,
		sizeof(int16_t) * 64
	};
	
	AudioInput audio_input { };
	ToneGen tone_gen { };
	ToneGen beep_gen { };
	dsp::modulate::Modulator *modulator = NULL ;

	bool am_enabled { false };
	bool fm_enabled { true };
	bool usb_enabled { false };
	bool lsb_enabled { false };
	bool dsb_enabled { false };

	uint32_t divider { };
	float audio_gain { };
	uint64_t power_acc { 0 };
	uint32_t power_acc_count { 0 };
	bool play_beep { false };
	uint32_t fm_delta { 0 };
	uint32_t phase { 0 }, sphase { 0 };
	int32_t sample { 0 }, delta { };
	uint32_t beep_index { }, beep_timer { };
	
	int8_t re { 0 }, im { 0 };
	
	AudioLevelReportMessage level_message { };
	TXProgressMessage txprogress_message { };
};

#endif
