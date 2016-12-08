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

#include "proc_closecall.hpp"

#include "event_m4.hpp"

#include <cstdint>
#include <cstddef>

#include <array>

void CloseCallProcessor::execute(const buffer_c8_t& buffer) {
	if( phase == 0 ) {
		std::fill(spectrum.begin(), spectrum.end(), 0);
	}

	for(size_t i=0; i<spectrum.size(); i++) {
		spectrum[i] += buffer.p[i +    0];
		spectrum[i] += buffer.p[i + 1024];
	}

	if( phase == 20 ) {
		const buffer_c16_t buffer_c16 {
			spectrum.data(),
			spectrum.size(),
			buffer.sampling_rate
		};
		channel_spectrum.feed(
			buffer_c16,
			0, 0
		);
		phase = 0;
	} else {
		phase++;
	}
}

void CloseCallProcessor::on_message(const Message* const message) {
	switch(message->id) {
	case Message::ID::UpdateSpectrum:
	case Message::ID::SpectrumStreamingConfig:
		channel_spectrum.on_message(message);
		break;
		
	default:
		break;
	}
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<CloseCallProcessor>() };
	event_dispatcher.run();
	return 0;
}
