/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2020 Shao
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

#include "proc_am_tv.hpp"

#include "portapack_shared_memory.hpp"
#include "dsp_fft.hpp"
#include "event_m4.hpp"

#include <cstdint>

void WidebandFMAudio::execute(const buffer_c8_t& buffer) {
	if( !configured ) {
		return;
	}
	
	std::fill(spectrum.begin(), spectrum.end(), 0);

	for(size_t i=0; i<spectrum.size(); i++) {
		spectrum[i] += buffer.p[i];
	}

	const buffer_c16_t buffer_c16 {spectrum.data(),spectrum.size(),buffer.sampling_rate};
	channel_spectrum.feed(buffer_c16);

        int8_t re, im;
	int8_t mag;

        for (size_t i = 0; i < 128; i++) 
	{
		re = buffer.p[i].real();
		im = buffer.p[i].imag();
		mag = __builtin_sqrtf((re * re) + (im * im)) ;
		const unsigned int v =  re + 127.0f; //timescope
		audio_spectrum.db[i] = std::max(0U, std::min(255U, v));
	}
	AudioSpectrumMessage message { &audio_spectrum };
	shared_memory.application_queue.push(message);
}

void WidebandFMAudio::on_message(const Message* const message) {
	switch(message->id) {
	case Message::ID::UpdateSpectrum:
	case Message::ID::SpectrumStreamingConfig:
		channel_spectrum.on_message(message);
		break;

	case Message::ID::WFMConfigure:
		configure(*reinterpret_cast<const WFMConfigureMessage*>(message));
		break;
		
	default:
		break;
	}
}

void WidebandFMAudio::configure(const WFMConfigureMessage& message) {
	configured = true;
}


int main() {
	EventDispatcher event_dispatcher { std::make_unique<WidebandFMAudio>() };
	event_dispatcher.run();
	return 0;
}
