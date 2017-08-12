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

#include "proc_adsbtx.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table_int8.hpp"
#include "event_m4.hpp"

#include <cstdint>

void ADSBTXProcessor::execute(const buffer_c8_t& buffer) {
	
	// This is called at 4M/2048 = 1953Hz
	// One pulse = 500ns = 2 samples
	// One bit = 2 pulses = 1us = 4 samples
	// Test this with ./dump1090 --freq 434000000 --gain 20
	// Or ./dump1090 --freq 434000000 --gain 20 --interactive --net --net-http-port 8080 --net-beast
	
	if (!configured) return;

	for (size_t i = 0; i < buffer.count; i++) {
		if (bit_pos >= (240 << 1)) {
			configured = false;
			cur_bit = 0;
		} else {
			cur_bit = shared_memory.bb_data.data[bit_pos >> 1];
			bit_pos++;
		}
		
		if (cur_bit) {
			// Crude AM
			buffer.p[i] = am_lut[phase & 3];
			phase++;
		} else {
			buffer.p[i] = { 0, 0 };
		}
	}
}

void ADSBTXProcessor::on_message(const Message* const p) {
	const auto message = *reinterpret_cast<const ADSBConfigureMessage*>(p);
	
	if (message.id == Message::ID::ADSBConfigure) {
		bit_pos = 0;
		phase = 0;
		configured = true;
	}
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<ADSBTXProcessor>() };
	event_dispatcher.run();
	return 0;
}
