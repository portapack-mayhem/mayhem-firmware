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
	//audio_output.write(audio);
	
	for (uint32_t c = 0; c < 16; c++) {
		
		const int32_t sample_int = audio.p[c] * 32768.0f;
		const int32_t audio_sample = __SSAT(sample_int, 16);
		
		slicer_sr <<= 1;
		if (phase == 0)
			slicer_sr |= (audio_sample < 0);		// Do we need hysteresis ?
		else
			slicer_sr |= !(audio_sample < 0);
			
		// Detect transitions to adjust clock
		if ((slicer_sr ^ (slicer_sr >> 1)) & 1) {
			if (sphase < (0x8000u - sphase_delta_half))
				sphase += sphase_delta_eighth;
			else
				sphase -= sphase_delta_eighth;
		}
		
		sphase += sphase_delta;
		
		// Symbol time elapsed
		if (sphase >= 0x10000u) {
			sphase &= 0xFFFFu;
			
			rx_data <<= 1;
			rx_data |= (slicer_sr & 1);
			
			switch (rx_state) {
				
				case WAITING:
					if (rx_data == 0xAAAAAAAA) {
						rx_state = PREAMBLE;
						sync_timeout = 0;
					}
					break;
				
				case PREAMBLE:
					if (sync_timeout < POCSAG_TIMEOUT) {
						sync_timeout++;

						if (rx_data == POCSAG_SYNCWORD) {
							packet.clear();
							codeword_count = 0;
							rx_bit = 0;
							msg_timeout = 0;
							rx_state = SYNC;
						}
						
					} else {
						// Timeout here is normal (end of message)
						rx_state = WAITING;
						//push_packet(pocsag::PacketFlag::TIMED_OUT);
					}
					break;
				
				case SYNC:
					if (msg_timeout < POCSAG_BATCH_LENGTH) {
						msg_timeout++;
						rx_bit++;
						
						if (rx_bit >= 32) {
							rx_bit = 0;
							
							// Got a complete codeword
							
							//pocsag_brute_repair(&s->l2.pocsag, &rx_data);
							
							packet.set(codeword_count, rx_data);
							
							if (codeword_count < 15) {
								codeword_count++;
							} else {
								push_packet(pocsag::PacketFlag::NORMAL);
								rx_state = PREAMBLE;
								sync_timeout = 0;
							}
						}
					} else {
						packet.set(0, codeword_count);	// Replace first codeword with count, for debug
						push_packet(pocsag::PacketFlag::TIMED_OUT);
						rx_state = WAITING;
					}
					break;

				default:
					break;
			}
		}
	}
}

void POCSAGProcessor::push_packet(pocsag::PacketFlag flag) {
	packet.set_bitrate(bitrate);
	packet.set_flag(flag);
	packet.set_timestamp(Timestamp::now());
	const POCSAGPacketMessage message(packet);
	shared_memory.application_queue.push(message);
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
	//audio_output.configure(false);

	bitrate = message.bitrate;
	phase = message.phase;
	sphase_delta = 0x10000u * bitrate / POCSAG_AUDIO_RATE;
	sphase_delta_half = sphase_delta / 2;			// Just for speed
	sphase_delta_eighth = sphase_delta / 8;
	
	rx_state = WAITING;
	configured = true;
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<POCSAGProcessor>() };
	event_dispatcher.run();
	return 0;
}
