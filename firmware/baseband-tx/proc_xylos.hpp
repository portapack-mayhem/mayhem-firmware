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

#ifndef __PROC_XYLOS_H__
#define __PROC_XYLOS_H__

#include "baseband_processor.hpp"

#define CCIR_TONELENGTH 15360-1 // 1536000/10/10
#define PHASEV 436.91	// (65536*1024)/1536000*10

class XylosProcessor : public BasebandProcessor {
public:
	void execute(buffer_c8_t buffer) override;

private:
	int16_t audio_data[64];

	const buffer_s16_t preview_audio_buffer {
		audio_data,
		sizeof(int16_t)*64
	};
	
	uint32_t ccir_phases[16] = {
								(uint32_t)(1981*PHASEV),
								(uint32_t)(1124*PHASEV),
								(uint32_t)(1197*PHASEV),
								(uint32_t)(1275*PHASEV),
								(uint32_t)(1358*PHASEV),
								(uint32_t)(1446*PHASEV),
								(uint32_t)(1540*PHASEV),
								(uint32_t)(1640*PHASEV),
								(uint32_t)(1747*PHASEV),
								(uint32_t)(1860*PHASEV),
								(uint32_t)(2400*PHASEV),
								(uint32_t)(930*PHASEV),
								(uint32_t)(2247*PHASEV),
								(uint32_t)(991*PHASEV),
								(uint32_t)(2110*PHASEV),
								(uint32_t)(1055*PHASEV)
							};

	int8_t re, im;
	uint8_t s, as = 0, ai;
    uint8_t byte_pos = 0;
    uint8_t digit = 0;
    uint32_t sample_count = CCIR_TONELENGTH;
	uint32_t aphase, phase, sphase;
	int32_t sample, frq;
	TXDoneMessage message;
};

#endif
