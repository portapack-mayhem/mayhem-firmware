/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
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

#include "proc_replay.hpp"

#include "event_m4.hpp"

#include "utility.hpp"

ReplayProcessor::ReplayProcessor() {

}

void ReplayProcessor::execute(const buffer_c8_t& buffer) {
	/* 2.4576MHz, 2048 samples */

	size_t pos = 0;
	
	for (size_t c = 0; c < 4; c++) {
		if( stream ) {
			const size_t bytes_to_read = sizeof(*buffer.p) * buffer.count / 4;
			const auto result = stream->read(iq_buffer.p, bytes_to_read);
		}

		//feed_channel_stats(channel);
		
		for (size_t i = 0; i < (buffer.count / 4); i++) {
			buffer.p[pos] = { iq_buffer.p[i].real() >> 8, iq_buffer.p[i].imag() >> 8 };
			pos++;
		}
	}
	
	/*spectrum_samples += channel.count;
	if( spectrum_samples >= spectrum_interval_samples ) {
		spectrum_samples -= spectrum_interval_samples;
		channel_spectrum.feed(channel, channel_filter_pass_f, channel_filter_stop_f);
	}*/
}

void ReplayProcessor::on_message(const Message* const message) {
	switch(message->id) {
	/*case Message::ID::UpdateSpectrum:
	case Message::ID::SpectrumStreamingConfig:
		channel_spectrum.on_message(message);
		break;*/

	case Message::ID::ReplayConfig:
		replay_config(*reinterpret_cast<const ReplayConfigMessage*>(message));
		break;

	default:
		break;
	}
}

void ReplayProcessor::replay_config(const ReplayConfigMessage& message) {
	if( message.config ) {
		stream = std::make_unique<StreamOutput>(message.config);
	} else {
		stream.reset();
	}
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<ReplayProcessor>() };
	event_dispatcher.run();
	return 0;
}
