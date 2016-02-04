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

#ifndef __PROC_PLAYAUDIO_H__
#define __PROC_PLAYAUDIO_H__

#include "baseband_processor.hpp"

class PlayAudioProcessor : public BasebandProcessor {
public:
	void execute(const buffer_c8_t& buffer) override;
	void fill_buffer(int8_t * inptr);

private:
	int8_t audio_fifo[4096];		// Probably too much (=85ms @ 48000Hz)
	uint16_t fifo_put, fifo_get;
	uint8_t as = 0, ai;
	int8_t re, im;
	
	int16_t st;
	
	int16_t audio_data[64];
	
	bool asked = false;

	const buffer_s16_t preview_audio_buffer {
		audio_data,
		sizeof(int16_t)*64
	};
	
	FIFOSignalMessage sigmessage;
	
	uint32_t aphase, phase, sphase;
	int32_t sample, frq;
};

#endif
