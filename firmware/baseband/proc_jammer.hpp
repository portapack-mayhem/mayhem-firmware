/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __PROC_JAMMER_H__
#define __PROC_JAMMER_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "portapack_shared_memory.hpp"
#include "jammer.hpp"

using namespace jammer;

class JammerProcessor : public BasebandProcessor {
public:
	void execute(const buffer_c8_t& buffer) override;
	
	void on_message(const Message* const msg) override;

private:
	bool configured { false };
	
	BasebandThread baseband_thread { 3072000, this, NORMALPRIO + 20, baseband::Direction::Transmit };
	
	JammerChannel * jammer_channels {  };
	
	JammerType noise_type { };
	uint32_t tone_delta { 0 }, lfsr { }, feedback { };
	uint32_t noise_period { 0 }, period_counter { 0 };
    uint32_t jammer_duration { 0 };
    uint32_t current_range { 0 };
	int64_t jammer_center { 0 }, jammer_bw { 0 };
    uint32_t sample_count { 0 };
	uint32_t aphase { 0 }, phase { 0 }, delta { 0 }, sphase { 0 };
	int8_t sample { 0 };
	int8_t re { 0 }, im { 0 };
	RetuneMessage message { };
};

#endif
