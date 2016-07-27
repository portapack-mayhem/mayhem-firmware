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

#ifndef __PROC_EPAR_H__
#define __PROC_EPAR_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"

#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"

#include "audio_output.hpp"

// One bit is 0.005119048s (~200bps ?)
// Time unit is 0.001706349s (586Hz)

#define EPAR_TU 262-1 		// 1536000/10/586
#define EPAR_SPACE 33-1
#define EPAR_REPEAT 26*2	// DEBUG

// 0: 011
// 1: 001

// Good: 1001110111111
// Bad:  1100111011111

class EPARProcessor : public BasebandProcessor {
public:
	void execute(const buffer_c8_t& buffer) override;

private:
	bool transmit_done;
	const int8_t bitdef[2][3] = {
		{-127, 127, 127},
		{-127, -127, 127}
	};
	int8_t re, im;
	uint8_t s;
	uint8_t state_length = 0;
	uint8_t current_bit = 0;
	uint8_t current_tu = 0;
    uint8_t bit_pos = 0;
    uint8_t repeat_count = 0;
    uint32_t sample_count = 0;
	uint32_t aphase, phase, sphase;
	int32_t sample, frq;
	uint8_t state = 1;
	TXDoneMessage message;
};

#endif
