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

#include "proc_afskrx.hpp"
#include "portapack_shared_memory.hpp"

#include "event_m4.hpp"

#include <cstdint>
#include <cstddef>

void AFSKRxProcessor::execute(const buffer_c8_t& buffer) {
	// This is called at 1500Hz

	if (!configured) return;
	
	const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
	const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
	const auto channel_out = channel_filter.execute(decim_1_out, dst_buffer);

	feed_channel_stats(channel_out);
	
	auto audio = demod.execute(channel_out, audio_buffer);

	for (size_t c = 0; c < audio.count; c++) {

		const int32_t sample_int = audio.p[c] * 32768.0f;
		const int32_t audio_sample = __SSAT(sample_int, 16);
		
		/*slicer_sr <<= 1;
		slicer_sr |= (audio_sample < 0);		// Do we need hysteresis ?

		// Detect transitions to adjust clock
		if ((slicer_sr ^ (slicer_sr >> 1)) & 1) {
			if (sphase < (0x8000u - sphase_delta_half))
				sphase += sphase_delta_eighth;
			else
				sphase -= sphase_delta_eighth;
		}
		
		sphase += sphase_delta;*/
		
		// Symbol time elapsed
		//if (sphase >= 0x10000u) {
		//	sphase &= 0xFFFFu;
			
			rx_data <<= 1;
			rx_data |= 1;
			
			bit_count++;
			if (bit_count == 8) {
				data_message.byte = rx_data;
				shared_memory.application_queue.push(data_message);
				bit_count = 0;
			}
		//}
	}
}

void AFSKRxProcessor::on_message(const Message* const message) {
	if (message->id == Message::ID::AFSKRxConfigure)
		configure(*reinterpret_cast<const AFSKRxConfigureMessage*>(message));
}

void AFSKRxProcessor::configure(const AFSKRxConfigureMessage& message) {
	constexpr size_t decim_0_input_fs = baseband_fs;
	constexpr size_t decim_0_output_fs = decim_0_input_fs / decim_0.decimation_factor;

	constexpr size_t decim_1_input_fs = decim_0_output_fs;
	constexpr size_t decim_1_output_fs = decim_1_input_fs / decim_1.decimation_factor;

	constexpr size_t channel_filter_input_fs = decim_1_output_fs;
	const size_t channel_filter_output_fs = channel_filter_input_fs / 2;

	const size_t demod_input_fs = channel_filter_output_fs;

	decim_0.configure(taps_16k0_decim_0.taps, 33554432);
	decim_1.configure(taps_16k0_decim_1.taps, 131072);
	channel_filter.configure(taps_16k0_channel.taps, 2);
	demod.configure(demod_input_fs, 5000);
	
	bitrate = message.bitrate;
	sphase_delta = 0x10000u * bitrate / 24000;
	sphase_delta_half = sphase_delta / 2;			// Just for speed
	sphase_delta_eighth = sphase_delta / 8;
	
	configured = true;
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<AFSKRxProcessor>() };
	event_dispatcher.run();
	return 0;
}
