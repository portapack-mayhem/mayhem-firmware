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

#include "proc_mictx.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table_int8.hpp"
#include "tonesets.hpp"
#include "event_m4.hpp"

#include <cstdint>

void MicTXProcessor::execute(const buffer_c8_t& buffer){

	// This is called at 1536000/2048 = 750Hz
	
	if (!configured) return;
	
	audio_input.read_audio_buffer(audio_buffer);
	modulator->set_gain_vumeter_beep(audio_gain, play_beep ) ;	
	modulator->execute(audio_buffer, buffer, configured, beep_index, beep_timer, txprogress_message, level_message, power_acc_count, divider );	// Now "Key Tones & CTCSS" baseband additon inside FM mod. dsp_modulate.cpp"
    
   /* Original fw 1.3.1  good reference, beep and vu-meter
	for (size_t i = 0; i < buffer.count; i++) {
		
		if (!play_beep) {
			sample = audio_buffer.p[i >> 6] >> 8;			// 1536000 / 64 = 24000
			sample *= audio_gain;
			
			power_acc += (sample < 0) ? -sample : sample;	// Power average for UI vu-meter
			
			if (power_acc_count) {
				power_acc_count--;
			} else {
				power_acc_count = divider;
				level_message.value = power_acc / (divider / 4);	// Why ?
				shared_memory.application_queue.push(level_message);
				power_acc = 0;
			}
		} else {
			if (beep_timer) {
				beep_timer--;
			} else {
				beep_timer = baseband_fs * 0.05;			// 50ms
				
				if (beep_index == BEEP_TONES_NB) {
					configured = false;
					shared_memory.application_queue.push(txprogress_message);
				} else {
					beep_gen.configure(beep_deltas[beep_index], 1.0);
					beep_index++;
				}
			}
			sample = beep_gen.process(0);    // TODO : Pending how to move inside modulate.cpp
		} 
	 */ 	
				
       /* Original fw 1.3.1  good reference FM moulation version, including "key tones CTCSS"  fw 1.3.1 
		sample = tone_gen.process(sample);
				
		// FM
		if (configured) {
			delta = sample * fm_delta;
			
			phase += delta;
			sphase = phase >> 24;

			re = (sine_table_i8[(sphase + 64) & 255]);
			im = (sine_table_i8[sphase]);
		} else {
			re = 0;
			im = 0;
		}
		
		buffer.p[i] = { re, im };
		
	}  */
}

void MicTXProcessor::on_message(const Message* const msg) {
	const AudioTXConfigMessage config_message = *reinterpret_cast<const AudioTXConfigMessage*>(msg);
	const RequestSignalMessage request_message = *reinterpret_cast<const RequestSignalMessage*>(msg);
	
	switch(msg->id) {
		case Message::ID::AudioTXConfig:
			if (fm_enabled) {
				 dsp::modulate::FM *fm = new dsp::modulate::FM();

				// Config fm_delta private var inside DSP modulate.cpp
				 fm->set_fm_delta(config_message.deviation_hz * (0xFFFFFFUL / baseband_fs));

				 // Config properly the private tone_gen function parameters inside DSP modulate.cpp  	
				 fm->set_tone_gen_configure(config_message.tone_key_delta, config_message.tone_key_mix_weight);		
				modulator = fm;
			}

			if (usb_enabled) {
				modulator = new dsp::modulate::SSB();
				modulator->set_mode(dsp::modulate::Mode::USB);
			}
			
			if (lsb_enabled) {
				modulator = new dsp::modulate::SSB();
				modulator->set_mode(dsp::modulate::Mode::LSB);
			}
			if (am_enabled) {
				modulator = new dsp::modulate::AM();
				modulator->set_mode(dsp::modulate::Mode::AM);
			}
			if (dsb_enabled) {
				modulator = new dsp::modulate::AM();
				modulator->set_mode(dsp::modulate::Mode::DSB);
			}

			modulator->set_over(baseband_fs / 24000); // Keep no change.
			
			am_enabled = config_message.am_enabled;
			usb_enabled = config_message.usb_enabled;
			lsb_enabled = config_message.lsb_enabled;
			dsb_enabled = config_message.dsb_enabled;
			if (!am_enabled || !usb_enabled || !lsb_enabled || !dsb_enabled) {
				fm_enabled = true;
			}
			
			audio_gain = config_message.audio_gain;
			divider = config_message.divider;
			power_acc_count = 0;

			// now this config  moved, in the case Message::ID::AudioTXConfig , only FM case.
			// tone_gen.configure(config_message.tone_key_delta, config_message.tone_key_mix_weight);	
			
			txprogress_message.done = true;

			play_beep = false;
			configured = true;
			break;
		
		case Message::ID::RequestSignal:
			if (request_message.signal == RequestSignalMessage::Signal::BeepRequest) {
				beep_index = 0;
				beep_timer = 0;
				play_beep = true;
			}
			break;

		default:
			break;
	}
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<MicTXProcessor>() };
	event_dispatcher.run();
	return 0;
}
