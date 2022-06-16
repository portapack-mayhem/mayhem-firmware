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

#ifndef __PROC_SSTV_TX_H__
#define __PROC_SSTV_TX_H__

#include "portapack_shared_memory.hpp"
#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "sstv.hpp"

using namespace sstv;

class SSTVTXProcessor : public BasebandProcessor {
public:
	void execute(const buffer_c8_t& buffer) override;
	
	void on_message(const Message* const p) override;

private:

	// 1900 300ms
	// 1200 10ms
	// 1900 300ms
	const std::pair<uint32_t, uint32_t> calibration_sequence[3] = {
		{ SSTV_F2D(1900), SSTV_MS2S(300) },
		{ SSTV_F2D(1200), SSTV_MS2S(10) },
		{ SSTV_F2D(1900), SSTV_MS2S(300) }
	};
	
	enum state_t {
		STATE_CALIBRATION = 0,
		STATE_VIS,
		STATE_SYNC,
		STATE_PIXELS
	};
	
	state_t state { };
	
	bool configured { false };
	
	BasebandThread baseband_thread { 3072000, this, NORMALPRIO + 20, baseband::Direction::Transmit };

	uint32_t vis_code_sequence[10] { };
	sstv_scanline scanline_buffer[2] { };
	uint8_t buffer_flip { 0 }, substep { 0 };
	uint32_t pixel_duration { };

	sstv_scanline * current_scanline { };
	
	uint8_t pixel_luma { };
	uint32_t fm_delta { 0 };
	uint32_t tone_phase { 0 };
	uint32_t tone_delta { 0 };
    uint32_t pixel_index { 0 };
    uint32_t sample_count { 0 };
	uint32_t phase { 0 }, sphase { 0 };
	int32_t tone_sample { 0 }, delta { 0 };
	int8_t re { }, im { };
	
	RequestSignalMessage sig_message { RequestSignalMessage::Signal::FillRequest };
};

#endif
