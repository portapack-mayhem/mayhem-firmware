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

#ifndef __PORTAPACK_SHARED_MEMORY_H__
#define __PORTAPACK_SHARED_MEMORY_H__

#include <cstdint>
#include <cstddef>

#include "message_queue.hpp"

struct JammerChannel {
	bool enabled;
	uint64_t center;
	uint32_t width;
	uint32_t duration;
};

struct ToneDef {
	uint32_t delta;
	uint32_t duration;
};
	
struct ToneData {
	ToneDef tone_defs[32];
	uint32_t silence;
	uint8_t message[256];
};

/* NOTE: These structures must be located in the same location in both M4 and M0 binaries */
struct SharedMemory {
	static constexpr size_t application_queue_k = 11;
	static constexpr size_t app_local_queue_k = 11;

	uint8_t application_queue_data[1 << application_queue_k] { 0 };
	uint8_t app_local_queue_data[1 << app_local_queue_k] { 0 };
	const Message* volatile baseband_message { nullptr };
	MessageQueue application_queue { application_queue_data, application_queue_k };
	MessageQueue app_local_queue { app_local_queue_data, app_local_queue_k };

	char m4_panic_msg[32] { 0 };
	
	union {
		ToneData tones_data;
		JammerChannel jammer_channels[24];
		uint8_t data[512];
	} bb_data { { { { 0, 0 } }, 0, { 0 } } };
};

extern SharedMemory& shared_memory;

#endif/*__PORTAPACK_SHARED_MEMORY_H__*/
