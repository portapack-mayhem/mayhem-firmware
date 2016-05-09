/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __PORTAPACK_SHARED_MEMORY_H__
#define __PORTAPACK_SHARED_MEMORY_H__

#include <cstdint>
#include <cstddef>

#include "message_queue.hpp"

struct TouchADCFrame {
	uint32_t dr[8];
};

struct JammerRange {
	bool active;
	int64_t center;
	int64_t width;
	uint32_t duration;
};

/* NOTE: These structures must be located in the same location in both M4 and M0 binaries */
struct SharedMemory {
	static constexpr size_t baseband_queue_k = 12;
	static constexpr size_t application_queue_k = 11;

	MessageQueue baseband_queue;
	uint8_t baseband_queue_data[1 << baseband_queue_k];
	MessageQueue application_queue;
	uint8_t application_queue_data[1 << application_queue_k];

	// TODO: M0 should directly configure and control DMA channel that is
	// acquiring ADC samples.
	TouchADCFrame touch_adc_frame;
	
	int test;
	
	uint32_t rdsdata[16];
	
	uint8_t lcrdata[256];
	uint32_t afsk_samples_per_bit;
	uint32_t afsk_phase_inc_mark;
	uint32_t afsk_phase_inc_space;
	uint8_t afsk_repeat;
	uint32_t afsk_fmmod;
	bool afsk_transmit_done;
	
	JammerRange jammer_ranges[16];
	
	char xylosdata[21];
	bool xylos_transmit_done;
};

extern SharedMemory& shared_memory;

#endif/*__PORTAPACK_SHARED_MEMORY_H__*/
