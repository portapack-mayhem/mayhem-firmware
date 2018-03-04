/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#ifndef __PROC_ADSBRX_H__
#define __PROC_ADSBRX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "adsb_frame.hpp"

using namespace adsb;

#define ADSB_PREAMBLE_LENGTH 16

class ADSBRXProcessor : public BasebandProcessor {
public:
	void execute(const buffer_c8_t& buffer) override;
	
	void on_message(const Message* const message) override;

private:
	static constexpr float k = 1.0f / 128.0f;
	
	static constexpr size_t baseband_fs = 2000000;
	
	BasebandThread baseband_thread { baseband_fs, this, NORMALPRIO + 20, baseband::Direction::Receive };
	RSSIThread rssi_thread { NORMALPRIO + 10 };
	
	ADSBFrame frame { };
	bool configured { false };
	float prev_mag { 0 };
	float threshold { }, threshold_low { }, threshold_high { };
	size_t null_count { 0 }, bit_count { 0 }, sample_count { 0 };
	std::pair<float, uint8_t> shifter[ADSB_PREAMBLE_LENGTH];
	bool decoding { };
	bool preamble { }, active { };
    uint16_t bit_pos { 0 };
    uint8_t cur_bit { 0 };
	uint32_t sample { 0 };
	int8_t re { }, im { };
};

#endif
