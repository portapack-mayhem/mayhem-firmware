/*
 * Copyright (C) 1996 Thomas Sailer (sailer@ife.ee.ethz.ch, hb9jnx@hb9w.che.eu)
 * Copyright (C) 2012-2014 Elias Oenal (multimon-ng@eliasoenal.com)
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

#include "proc_pocsag.hpp"

#include "event_m4.hpp"

#include <cstdint>
#include <cstddef>

void POCSAGProcessor::execute(const buffer_c8_t& buffer) {
	// This is called at 1500Hz
	
	if (!configured) return;
	
	// Get 24kHz audio
	const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
	const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
	const auto channel_out = channel_filter.execute(decim_1_out, dst_buffer);
	auto audio = demod.execute(channel_out, audio_buffer);
	smooth.Process(audio.p, audio.count); // Smooth the data to  make decoding more accurate
	audio_output.write(audio);
	
	processDemodulatedSamples(audio.p, 16);
	extractFrames();

}

// ====================================================================
//
// ====================================================================
int POCSAGProcessor::OnDataWord(uint32_t word, int pos)
{
	packet.set(pos, word);
	return 0;
}

// ====================================================================
//
// ====================================================================
int POCSAGProcessor::OnDataFrame(int len, int baud)
{
	if (len > 0)
	{
		packet.set_bitrate(baud);
		packet.set_flag(pocsag::PacketFlag::NORMAL);
		packet.set_timestamp(Timestamp::now());
		const POCSAGPacketMessage message(packet);
		shared_memory.application_queue.push(message);
	}
	return 0;
}

void POCSAGProcessor::on_message(const Message* const message) {
	if (message->id == Message::ID::POCSAGConfigure)
		configure(*reinterpret_cast<const POCSAGConfigureMessage*>(message));
}

void POCSAGProcessor::configure(const POCSAGConfigureMessage& message) {
	constexpr size_t decim_0_input_fs = baseband_fs;
	constexpr size_t decim_0_output_fs = decim_0_input_fs / decim_0.decimation_factor;

	constexpr size_t decim_1_input_fs = decim_0_output_fs;
	constexpr size_t decim_1_output_fs = decim_1_input_fs / decim_1.decimation_factor;
	
	constexpr size_t channel_filter_input_fs = decim_1_output_fs;
	const size_t channel_filter_output_fs = channel_filter_input_fs / 2;

	const size_t demod_input_fs = channel_filter_output_fs;

	decim_0.configure(taps_11k0_decim_0.taps, 33554432);
	decim_1.configure(taps_11k0_decim_1.taps, 131072);
	channel_filter.configure(taps_11k0_channel.taps, 2);
	demod.configure(demod_input_fs, 4500);
	// Smoothing should be roughly sample rate over max baud
	// 24k / 3.2k is 7.5
	smooth.SetSize(8);
	audio_output.configure(false);

	// Set up the frame extraction, limits of baud
	setFrameExtractParams(demod_input_fs, 4000, 300, 32);

	// Mark the class as ready to accept data
	configured = true;
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<POCSAGProcessor>() };
	event_dispatcher.run();
	return 0;
}
