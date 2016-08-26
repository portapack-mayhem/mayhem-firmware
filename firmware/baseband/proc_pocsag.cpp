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

#define FREQ_SAMP		24000
#define BAUD			1200
#define SPHASEINC		(0x10000u * BAUD / FREQ_SAMP)

#define POCSAG_SYNC		0x7CD215D8
#define POCSAG_IDLE		0x7A89C197

void POCSAGProcessor::execute(const buffer_c8_t& buffer) {
	/* 1.536MHz, 2048 samples */
	
	if( !configured ) return;
	
	const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
	const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);

	auto audio = demod.execute(decim_1_out, audio_buffer);
	
	for (c = 0; c < 32; c++) {
		
		// Bit = sign
		const int32_t audio_sample = audio.p[c] * 32768.0f;	// sample_int
		//const int32_t audio_sample = __SSAT(sample_int, 16);
		
		dcd_shreg <<= 1;
		dcd_shreg |= (audio_sample < 0);

		// Detect transitions to adjust clock
		if ((dcd_shreg ^ (dcd_shreg >> 1)) & 1) {
			if (sphase < (0x8000u-(SPHASEINC/2)))
				sphase += SPHASEINC/8;
			else
				sphase -= SPHASEINC/8;
		}
		
		sphase += SPHASEINC;
		
		if (sphase >= 0x10000u) {
			sphase &= 0xffffu;
			
			rx_data <<= 1;
			rx_data |= (dcd_shreg & 1);
			
			switch(rx_state) {
				
				case WAITING:
					if (rx_data == 0xAAAAAAAA) {
						rx_state = PREAMBLE;
						sync_timeout = 0;
					}
					break;
						
				case PREAMBLE:
					if (sync_timeout < 600) {
						sync_timeout++;
						
						//pocsag_brute_repair(&s->l2.pocsag, &rx_data);

						if (rx_data == POCSAG_SYNC) {
							packet.clear();
							frame_counter = 0;
							rx_bit = 0;
							msg_timeout = 0;
							last_rx_data = rx_data;
							rx_state = SYNC;
						} else if (rx_data == POCSAG_IDLE) {
							rx_state = WAITING;
						}
						
					} else {
						rx_state = WAITING;		// Abort
					}
					break;
				
				case SYNC:
					if (msg_timeout < 600) {
						msg_timeout++;
						rx_bit++;
						
						if (rx_bit >= 32) {
							rx_bit = 0;
							
							//pocsag_brute_repair(&s->l2.pocsag, &rx_data);
							
							packet.set(frame_counter, rx_data);
							
							if ((rx_data == POCSAG_IDLE) && (!(last_rx_data & 0x80000000))) {
								// SYNC then IDLE always means end of message ?
								packet.set_bitrate(pocsag::BitRate::FSK1200);
								packet.set_flag(pocsag::PacketFlag::NORMAL);
								packet.set_timestamp(Timestamp::now());
								const POCSAGPacketMessage message(packet);
								shared_memory.application_queue.push(message);
								rx_state = WAITING;
							} else {
								if (frame_counter < 15) {
									frame_counter++;
								} else {
									// More than 17-1 codewords
									packet.set_bitrate(pocsag::BitRate::FSK1200);
									packet.set_flag(pocsag::PacketFlag::TOO_LONG);
									packet.set_timestamp(Timestamp::now());
									const POCSAGPacketMessage message(packet);
									shared_memory.application_queue.push(message);
									rx_state = WAITING;
								}
							}
							
							last_rx_data = rx_data;
						}
					} else {
						// Timed out (no end of message received)
						packet.set_bitrate(pocsag::BitRate::FSK1200);
						packet.set_flag(pocsag::PacketFlag::TIMED_OUT);
						packet.set_timestamp(Timestamp::now());
						const POCSAGPacketMessage message(packet);
						shared_memory.application_queue.push(message);
						rx_state = WAITING;
					}
					break;

				default:
					break;
			}
		}
	}
}

void POCSAGProcessor::on_message(const Message* const message) {
	if (message->id == Message::ID::POCSAGConfigure)
		configure(*reinterpret_cast<const POCSAGConfigureMessage*>(message));
}

void POCSAGProcessor::configure(const POCSAGConfigureMessage& message) {
	(void)message;
	
	constexpr size_t decim_0_input_fs = baseband_fs;
	constexpr size_t decim_0_output_fs = decim_0_input_fs / decim_0.decimation_factor;

	constexpr size_t decim_1_input_fs = decim_0_output_fs;
	constexpr size_t decim_1_output_fs = decim_1_input_fs / decim_1.decimation_factor;

	const size_t demod_input_fs = decim_1_output_fs;

	decim_0.configure(taps_11k0_decim_0.taps, 33554432);
	decim_1.configure(taps_11k0_decim_1.taps, 131072);
	demod.configure(demod_input_fs, 4500);

	rx_state = WAITING;
	configured = true;
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<POCSAGProcessor>() };
	event_dispatcher.run();
	return 0;
}
