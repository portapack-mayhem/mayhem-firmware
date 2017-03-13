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

#include "audio_input.hpp"
#include "baseband_processor.hpp"
#include "baseband_thread.hpp"

class MicTXProcessor : public BasebandProcessor {
public:
	void execute(const buffer_c8_t& buffer) override;
	
	void on_message(const Message* const msg) override;

private:
	bool configured = false;
	
	BasebandThread baseband_thread { 1536000, this, NORMALPRIO + 20, baseband::Direction::Transmit };
	
	int16_t audio_data[64];
	buffer_s16_t audio_buffer {
		audio_data,
		sizeof(int16_t) * 64
	};
	
	AudioInput audio_input { };
	
	uint32_t divider { }, gain_x10 { };
	uint32_t as { 0 };
	uint32_t fm_delta { 0 };
	bool ctcss_enabled { false };
	uint32_t ctcss_phase_inc { };
	uint32_t ctcss_phase { 0 }, phase { 0 }, sphase { 0 };
	int32_t ctcss_sample { 0 }, sample { 0 }, sample_mixed { }, delta { };
	uint64_t power { 0 };
	
	int8_t re { 0 }, im { 0 };
	
	AudioLevelMessage level_message { };
};

#endif
