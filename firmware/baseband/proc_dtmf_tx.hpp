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

#ifndef __PROC_DTMFTX_H__
#define __PROC_DTMFTX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"

#define DTMF_PHASEINC	436.91		// (65536*1024)/1536000*10
#define DTMF_C0			(uint32_t)(1209*DTMF_PHASEINC)
#define DTMF_C1			(uint32_t)(1336*DTMF_PHASEINC)
#define DTMF_C2			(uint32_t)(1477*DTMF_PHASEINC)
#define DTMF_C3			(uint32_t)(1633*DTMF_PHASEINC)
#define DTMF_R0			(uint32_t)(697*DTMF_PHASEINC)
#define DTMF_R1			(uint32_t)(770*DTMF_PHASEINC)
#define DTMF_R2			(uint32_t)(852*DTMF_PHASEINC)
#define DTMF_R3			(uint32_t)(941*DTMF_PHASEINC)

class DTMFTXProcessor : public BasebandProcessor {
public:
	void execute(const buffer_c8_t& buffer) override;
	
	void on_message(const Message* const msg) override;

private:
	bool configured = false;
	
	BasebandThread baseband_thread { 1536000, this, NORMALPRIO + 20, baseband::Direction::Transmit };
	
	// 0123456789ABCD#*
	const uint32_t DTMF_LUT[16][2] = {
		{ DTMF_C1, DTMF_R3 },
		{ DTMF_C0, DTMF_R0 },
		{ DTMF_C1, DTMF_R0 },
		{ DTMF_C2, DTMF_R0 },
		{ DTMF_C0, DTMF_R1 },
		{ DTMF_C1, DTMF_R1 },
		{ DTMF_C2, DTMF_R1 },
		{ DTMF_C0, DTMF_R2 },
		{ DTMF_C1, DTMF_R2 },
		{ DTMF_C2, DTMF_R2 },
		{ DTMF_C3, DTMF_R0 },
		{ DTMF_C3, DTMF_R1 },
		{ DTMF_C3, DTMF_R2 },
		{ DTMF_C3, DTMF_R3 },
		{ DTMF_C2, DTMF_R3 },
		{ DTMF_C0, DTMF_R3 }
	};
	
	uint32_t tone_length, pause_length;
	uint32_t as, bw;
	uint8_t tone_idx = 0, tone_code = 0;

	uint32_t timer = 0;
	
	bool tone = false;		// Tone / pause
	
	int8_t re, im;
	int8_t sample;

	//int16_t audio_data[64];
	/*const buffer_s16_t preview_audio_buffer {
		audio_data,
		sizeof(int16_t)*64
	};*/
	
	TXDoneMessage txdone_message;
	
	uint32_t tone_a_phase, tone_b_phase, phase, sphase;
	int32_t frq;
};

#endif
