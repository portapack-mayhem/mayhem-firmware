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

void AFSKRxProcessor::execute(const buffer_c8_t& buffer) {
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
		sample_bits |= (sample_filtered < -20) ? 1 : 0;
		
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
			
			if (trigger_word) {
				
				// Continuous-stream value-triggered mode (AX.25) - UNTESTED
				word_bits <<= 1;
				word_bits |= (sample_bits & 1);
				
				bit_counter++;
				
				if (triggered) {
					if (bit_counter == word_length) {
						bit_counter = 0;
						
						data_message.is_data = true;
						data_message.value = word_bits & word_mask;
						shared_memory.application_queue.push(data_message);
					}
				} else {
					if ((word_bits & word_mask) == trigger_value) {
						triggered = !triggered;
						bit_counter = 0;
						
						data_message.is_data = true;
						data_message.value = trigger_value;
						shared_memory.application_queue.push(data_message);
					}
				}
				
			} else {
				
				// RS232-like modem mode
				if (state == WAIT_START) {
					if (!(sample_bits & 1)) {
						// Got start bit
						state = RECEIVE;
						bit_counter = 0;
					}
				} else if (state == WAIT_STOP) {
					if (sample_bits & 1) {
						// Got stop bit
						state = WAIT_START;
					}
				} else {
					word_bits <<= 1;
					word_bits |= (sample_bits & 1);
					
					bit_counter++;
				}
				
				if (bit_counter == word_length) {
					bit_counter = 0;
					state = WAIT_STOP;
					
					data_message.is_data = true;
					data_message.value = word_bits;
					shared_memory.application_queue.push(data_message);
				}
				
			}
		}
	}
}

void AFSKRxProcessor::on_message(const Message* const message) {
	if (message->id == Message::ID::AFSKRxConfigure)
		configure(*reinterpret_cast<const AFSKRxConfigureMessage*>(message));
}

void AFSKRxProcessor::configure(const AFSKRxConfigureMessage& message) {
	/*constexpr size_t decim_0_input_fs = baseband_fs;
	constexpr size_t decim_0_output_fs = decim_0_input_fs / decim_0.decimation_factor;

	constexpr size_t decim_1_input_fs = decim_0_output_fs;
	constexpr size_t decim_1_output_fs = decim_1_input_fs / decim_1.decimation_factor;
	
	constexpr size_t channel_filter_input_fs = decim_1_output_fs;
	const size_t channel_filter_output_fs = channel_filter_input_fs / 2;

	const size_t demod_input_fs = channel_filter_output_fs;*/
	
	decim_0.configure(taps_11k0_decim_0.taps, 33554432);
	decim_1.configure(taps_11k0_decim_1.taps, 131072);
	channel_filter.configure(taps_11k0_channel.taps, 2);
	demod.configure(audio_fs, 5000);

	audio_output.configure(audio_24k_hpf_300hz_config, audio_24k_deemph_300_6_config, 0);
	
	samples_per_bit = audio_fs / message.baudrate;
	
	phase_inc = (0x10000 * message.baudrate) / audio_fs;
	phase = 0;
	
	trigger_word = message.trigger_word;
	word_length = message.word_length;
	trigger_value = message.trigger_value;
	word_mask = (1 << word_length) - 1;
	
	// Delay line
	delay_line_index = 0;
	
	triggered = false;
	state = WAIT_START;
	
	configured = true;
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<AFSKRxProcessor>() };
	event_dispatcher.run();
	return 0;
}
