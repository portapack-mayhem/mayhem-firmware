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

#include "message_queue.hpp"

struct TouchADCFrame {
	uint32_t dr[8];
};

/* NOTE: These structures must be located in the same location in both M4 and M0 binaries */
struct SharedMemory {
	MessageQueue baseband_queue;
	MessageQueue application_queue;

	// TODO: M0 should directly configure and control DMA channel that is
	// acquiring ADC samples.
	TouchADCFrame touch_adc_frame;
	int8_t correction_ppm;
};

extern SharedMemory& shared_memory;

#if defined(LPC43XX_M0)
inline void init_message_queues() {
	new (&shared_memory.baseband_queue) MessageQueue();
	new (&shared_memory.application_queue) MessageQueue();
}
#endif

#endif/*__PORTAPACK_SHARED_MEMORY_H__*/
