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

#ifndef __PROC_TONES_H__
#define __PROC_TONES_H__

#include "portapack_shared_memory.hpp"
#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "audio_output.hpp"

class TonesProcessor : public BasebandProcessor {
public:
	void execute(const buffer_c8_t& buffer) override;
	
	void on_message(const Message* const p) override;

private:
	bool configured = false;
	
	BasebandThread baseband_thread { 1536000, this, NORMALPRIO + 20, baseband::Direction::Transmit };
	
	std::array<int16_t, 32> audio { };		// 2048/64
	const buffer_s16_t audio_buffer {
		(int16_t*)audio.data(),
		sizeof(audio) / sizeof(int16_t)
	};

	uint32_t tone_deltas[32];
	uint32_t tone_durations[32];
	
	bool audio_out { false };
	bool dual_tone { false };
	uint32_t fm_delta { 0 };
	uint32_t tone_a_phase { 0 }, tone_b_phase { 0 };
	uint32_t tone_a_delta { 0 }, tone_b_delta { 0 };
    uint8_t digit_pos { 0 };
    uint8_t digit { 0 };
    uint32_t silence_count { 0 }, sample_count { 0 };
    uint32_t message_length { 0 };
	uint32_t phase { 0 }, sphase { 0 };
	int32_t tone_sample { 0 }, delta { 0 };
	int8_t re { 0 }, im { 0 };
	uint8_t as { 0 }, ai { 0 };
	
	TXProgressMessage txprogress_message { };
	AudioOutput audio_output { };
};

#endif
