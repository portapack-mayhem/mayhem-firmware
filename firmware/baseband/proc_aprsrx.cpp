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

#include "proc_aprsrx.hpp"
#include "portapack_shared_memory.hpp"

#include "event_m4.hpp"

#include "stdio.h"

void APRSRxProcessor::execute(const buffer_c8_t& buffer) {
	// This is called at 3072000 / 2048 = 1500Hz

	if (!configured) return;
	
	// FM demodulation
	const auto decim_0_out = decim_0.execute(buffer, dst_buffer);				// 2048 / 8 = 256 (512 I/Q samples)
	const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);			// 256 / 8 = 32 (64 I/Q samples)
	const auto channel_out = channel_filter.execute(decim_1_out, dst_buffer);	// 32 / 2 = 16 (32 I/Q samples)

	feed_channel_stats(channel_out);
	
	auto audio = demod.execute(channel_out, audio_buffer);
	
	audio_output.write(audio);

	// Audio signal processing
	for (size_t c = 0; c < audio.count; c++) {

		const int32_t sample_int = audio.p[c] * 32768.0f;
		int32_t current_sample = __SSAT(sample_int, 16);
		
		current_sample /= 128;
		
		// Delay line put
		delay_line[delay_line_index & 0x3F] = current_sample;
		
		// Delay line get, and LPF
		sample_mixed = (delay_line[(delay_line_index - (samples_per_bit/2)) & 0x3F] * current_sample) / 4; 
		sample_filtered = prev_mixed + sample_mixed + (prev_filtered / 2);
		
		delay_line_index++;
		
		prev_filtered = sample_filtered;
		prev_mixed = sample_mixed;		

		// Slice
		sample_bits <<= 1;

		uint8_t bit = (sample_filtered < -20) ? 1 : 0;
		sample_bits |= bit;

/*
		int16_t scaled = bit == 1 ? 32767 : -32767;

		if( stream ) {
			const size_t bytes_to_write = sizeof(scaled) * 1;
			const auto result = stream->write(&scaled, bytes_to_write);
		}
*/
		
		// Check for "clean" transition: either 0011 or 1100
		if ((((sample_bits >> 2) ^ sample_bits) & 3) == 3) {
			// Adjust phase
			if (phase < 0x8000)
				phase += 0x800;		// Is this a proper value ?
			else
				phase -= 0x800;
		}
		
		phase += phase_inc;
		
		if (phase >= 0x10000) {
			phase &= 0xFFFF;
			
			if (true) {			
				uint8_t bit;					
				if(__builtin_popcount(sample_bits & 0xFF) >= 0x05){
					bit = 0x1;
				}
				else {
					bit = 0x0;
				}

				if(parse_bit(bit)){
					parse_packet();
				}						
			}
		}
	}
}

void APRSRxProcessor::parse_packet(){
	//validate crc	
	if(packet_buffer_size >= aprs::APRS_MIN_LENGTH){
		uint16_t crc = 0xFFFF;

		for(size_t i = 0; i < packet_buffer_size; i++){			
			uint8_t byte = packet_buffer[i];
			crc = ((crc >> 8) ^ crc_ccitt_tab[(crc ^ byte) & 0xFF]) & 0xFFFF;
		}	

		if(crc == 0xF0B8){		
			parse_ax25();
		}
	}	
}

void APRSRxProcessor::parse_ax25(){	
	aprs_packet.clear();
	aprs_packet.set_valid_checksum(true);

	for(size_t i = 0; i < packet_buffer_size; i++){
		aprs_packet.set(i, packet_buffer[i]);
	}

	APRSPacketMessage packet_message { aprs_packet };
	shared_memory.application_queue.push(packet_message);
}

bool APRSRxProcessor::parse_bit(const uint8_t current_bit){
	uint8_t decoded_bit = ~(current_bit ^ last_bit) & 0x1;
	last_bit = current_bit;

	int16_t log = decoded_bit == 0 ? -32768 : 32767;
				
	//if( stream ) {
	//	const size_t bytes_to_write = sizeof(log) * 1;
//		const auto result = stream->write(&log, bytes_to_write);
//	}

	if(decoded_bit & 0x1){
		if(ones_count < 8){
			ones_count++;
		}
	}
	else {
		if(ones_count > 6){ //not valid
			state = WAIT_FLAG;
			current_byte = 0;
			ones_count = 0;
			byte_index = 0;
			packet_buffer_size = 0;
			return false;
		}
		else if(ones_count == 6){ //flag
			bool done = false;
			if(state == IN_FRAME){				
				done = true;
			}
			else {
				packet_buffer_size = 0;
			}
			state = WAIT_FRAME;
			current_byte = 0;
			ones_count = 0;
			byte_index = 0;			

			return done;
		}
		else if(ones_count == 5){ //bit stuff
			ones_count = 0;
			return false;
		}
		else {
			ones_count = 0;
		}
	}

	//store
	current_byte = current_byte >> 1;
	current_byte |= (decoded_bit == 0x1 ? 0x80 : 0x0);
	byte_index++;

	if(byte_index >= 8){
		byte_index = 0;
		if(state == WAIT_FRAME){
			state = IN_FRAME;
		}

		if(state == IN_FRAME){	
			if(packet_buffer_size + 1 >= 256){
				state = WAIT_FLAG;
				current_byte = 0;
				ones_count = 0;
				byte_index = 0;
				packet_buffer_size = 0;
				return false;
			}
			packet_buffer[packet_buffer_size++] = current_byte;
		}
	}

	return false;
}

void APRSRxProcessor::on_message(const Message* const message) {
	if (message->id == Message::ID::APRSRxConfigure)
		configure(*reinterpret_cast<const APRSRxConfigureMessage*>(message));
	if(message->id == Message::ID::CaptureConfig)
		capture_config(*reinterpret_cast<const CaptureConfigMessage*>(message));	
}

void APRSRxProcessor::capture_config(const CaptureConfigMessage& message) {
	if( message.config ) {
		//stream = std::make_unique<StreamInput>(message.config);
		audio_output.set_stream(std::make_unique<StreamInput>(message.config));
	} else {
		//stream.reset();
		audio_output.set_stream(nullptr);		
	}
}

void APRSRxProcessor::configure(const APRSRxConfigureMessage& message) {	
	decim_0.configure(taps_11k0_decim_0.taps, 33554432);
	decim_1.configure(taps_11k0_decim_1.taps, 131072);
	channel_filter.configure(taps_11k0_channel.taps, 2);
	demod.configure(audio_fs, 5000);

	audio_output.configure(audio_24k_hpf_300hz_config, audio_24k_deemph_300_6_config, 0);
	
	samples_per_bit = audio_fs / message.baudrate;
	
	phase_inc = (0x10000 * message.baudrate) / audio_fs;
	phase = 0;
	
	// Delay line
	delay_line_index = 0;

	state = WAIT_FLAG;
	
	configured = true;
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<APRSRxProcessor>() };
	event_dispatcher.run();
	return 0;
}
