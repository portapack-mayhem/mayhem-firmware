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

#include "ui_mictx.hpp"

#include "baseband_api.hpp"
#include "audio.hpp"

#include "wm8731.hpp"
using wolfson::wm8731::WM8731;

#include "tonesets.hpp"
#include "portapack_hal.hpp"
#include "string_format.hpp"
#include "irq_controls.hpp"

#include <cstring>

using namespace tonekey;
using namespace portapack;


WM8731 audio_codec_wm8731 { i2c0, 0x1a };


namespace ui {

void MicTXView::focus() {
	switch(focused_ui) {
		case 0:
			field_frequency.focus();
			break;
		case 1:
			field_rxfrequency.focus();
			break;
		default:
			field_va.focus();
			break;
	}	
}

void MicTXView::update_vumeter() {
	vumeter.set_value(audio_level);
}

void MicTXView::on_tx_progress(const bool done) {
	// Roger beep played, stop transmitting
	if (done)
		set_tx(false);
}

void MicTXView::configure_baseband() {
	baseband::set_audiotx_config(
		sampling_rate / 20,		// Update vu-meter at 20Hz
		transmitting ? transmitter_model.channel_bandwidth() : 0,
		mic_gain,
		TONES_F2D(tone_key_frequency(tone_key_index), sampling_rate),
		enable_am,
		enable_dsb,
		enable_usb,
		enable_lsb
	);

}

void MicTXView::set_tx(bool enable) {
	if (enable) {
		if (rx_enabled)  //If audio RX is enabled
			rxaudio(false); //Then turn off audio RX
		transmitting = true;
		configure_baseband();
		transmitter_model.set_tuning_frequency(tx_frequency);
		transmitter_model.set_tx_gain(tx_gain);
		transmitter_model.set_rf_amp(rf_amp);
		transmitter_model.enable();
		portapack::pin_i2s0_rx_sda.mode(3);		// This is already done in audio::init but gets changed by the CPLD overlay reprogramming
	} else {
		if (transmitting && rogerbeep_enabled) {
			baseband::request_beep();	//Transmit the roger beep
			transmitting = false;		//And flag the end of the transmission so ...
		} else { // (if roger beep was enabled, this will be executed after the beep ends transmitting.
			transmitting = false;
			configure_baseband();
			transmitter_model.disable();
			if (rx_enabled)  //If audio RX is enabled and we've been transmitting
				rxaudio(true); //Turn back on audio RX
		}
	}
}

void MicTXView::do_timing() {
	if (va_enabled) {
		if (!transmitting) {
			// Attack
			if (audio_level >= va_level) {
				if ((attack_timer >> 8) >= attack_ms) {
					decay_timer = 0;
					attack_timer = 0;
					set_tx(true);
				} else {
					attack_timer += lcd_frame_duration;
				}
			} else {
				attack_timer = 0;
			}
		} else {
			// Decay
			if (audio_level < va_level) {
				if ((decay_timer >> 8) >= decay_ms) {
					decay_timer = 0;
					attack_timer = 0;
					set_tx(false);
				} else {
					decay_timer += lcd_frame_duration;
				}
			} else {
				decay_timer = 0;
			}
		}
	} else {
		// Check for PTT release
		const auto switches_state = get_switches_state();
		if (!switches_state[4] && transmitting && !button_touch)		// Select button
			set_tx(false);
	}
}

/* Hmmmm. Maybe useless now.
void MicTXView::on_tuning_frequency_changed(rf::Frequency f) {
	transmitter_model.set_tuning_frequency(f);
	//if ( rx_enabled )
		receiver_model.set_tuning_frequency(f); //Update freq also for RX
}
*/

void MicTXView::rxaudio(bool is_on) {
	if (is_on) {
		audio::input::stop();
		baseband::shutdown();
		
		if (enable_am || enable_usb || enable_lsb || enable_dsb) {
			baseband::run_image(portapack::spi_flash::image_tag_am_audio);
			receiver_model.set_modulation(ReceiverModel::Mode::AMAudio);	
			if (options_mode.selected_index() < 4)
				receiver_model.set_am_configuration(options_mode.selected_index() - 1);
			else
				receiver_model.set_am_configuration(0);
		}
		else {
			baseband::run_image(portapack::spi_flash::image_tag_nfm_audio);
			receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio);
			
		}
		receiver_model.set_sampling_rate(3072000);
		receiver_model.set_baseband_bandwidth(1750000);	
//		receiver_model.set_tuning_frequency(field_frequency.value()); //probably this too can be commented out.
		receiver_model.set_tuning_frequency(rx_frequency); // Now with seperate controls!
		receiver_model.set_lna(rx_lna);
		receiver_model.set_vga(rx_vga);
		receiver_model.set_rf_amp(rx_amp);
		receiver_model.enable();
		audio::output::start();
	} else {	//These incredibly convoluted steps are required for the vumeter to reappear when stopping RX.
		receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio); //This fixes something with AM RX...
		receiver_model.disable();
		baseband::shutdown();

		baseband::run_image(portapack::spi_flash::image_tag_mic_tx);
		audio::output::stop();
		audio::input::start();	// set up audio input =  mic config of any audio coded AK4951/WM8731, (in WM8731 parameter will be ignored)
		portapack::pin_i2s0_rx_sda.mode(3);
		configure_baseband();
	}
}

void MicTXView::on_headphone_volume_changed(int32_t v) {
	//if (rx_enabled) {
		const auto new_volume = volume_t::decibel(v - 99) + audio::headphone::volume_range().max;
		receiver_model.set_headphone_volume(new_volume);
	//}
}

void MicTXView::set_ptt_visibility(bool v) {
	tx_button.hidden(!v);
}

MicTXView::MicTXView(
	NavigationView& nav
)
{
	portapack::pin_i2s0_rx_sda.mode(3);		// This is already done in audio::init but gets changed by the CPLD overlay reprogramming
	
	baseband::run_image(portapack::spi_flash::image_tag_mic_tx);
	
	
	if (true ) {       // Temporary , disabling ALC feature , (pending to solve -No Audio in Mic app ,in some H2/H2+ WM /QFP100 CPLS users-  if ( audio_codec_wm8731.detected() ) {
	add_children({		
		&labels_WM8731,		// we have audio codec WM8731, same MIC menu  as original.
		&vumeter,
		&options_gain,		// MIC GAIN float factor on the GUI.
//		&check_va,
		&field_va,
		&field_va_level,
		&field_va_attack,
		&field_va_decay,
		&field_bw,
		&field_rfgain,
		&field_rfamp,
		&options_mode,
		&field_frequency,
		&options_tone_key,
		&check_rogerbeep,
		&check_rxactive,
		&field_volume,
		&field_rxbw,
		&field_squelch,
		&field_rxfrequency,
		&field_rxlna,
		&field_rxvga,
		&field_rxamp,
		&tx_button
	});
    
	} else {			
	add_children({
		&labels_AK4951,		// we have audio codec AK4951, enable Automatic Level Control
		&vumeter,
		&options_gain,
		&options_ak4951_alc_mode,
//		&check_va,
		&field_va,
		&field_va_level,
		&field_va_attack,
		&field_va_decay,
		&field_bw,
		&field_rfgain,
		&field_rfamp,
		&options_mode,
		&field_frequency,
		&options_tone_key,
		&check_rogerbeep,
		&check_rxactive,
		&field_volume,
		&field_rxbw,
		&field_squelch,
		&field_rxfrequency,
		&field_rxlna,
		&field_rxvga,
		&field_rxamp,
		&tx_button
	});
	
	}

	tone_keys_populate(options_tone_key);
	options_tone_key.on_change = [this](size_t i, int32_t) {
		tone_key_index = i;
	};
	options_tone_key.set_selected_index(0);
	
	options_gain.on_change = [this](size_t, int32_t v) {
		mic_gain = v / 10.0;
		configure_baseband();
	};
	options_gain.set_selected_index(1);		// x1.0
	
	options_ak4951_alc_mode.on_change = [this](size_t, int8_t v) {
	      ak4951_alc_GUI_selected  = v;
		  audio::input::start();
	};
	// options_ak4951_alc_mode.set_selected_index(0);
	 
	tx_frequency = transmitter_model.tuning_frequency();
	field_frequency.set_value(transmitter_model.tuning_frequency());
	field_frequency.set_step(receiver_model.frequency_step());
	field_frequency.on_change = [this](rf::Frequency f) {
		tx_frequency = f;
		if(!rx_enabled)
			transmitter_model.set_tuning_frequency(f);
	};
	field_frequency.on_edit = [this, &nav]() {
		focused_ui = 0;
		// TODO: Provide separate modal method/scheme?
		auto new_view = nav.push<FrequencyKeypadView>(tx_frequency);
		new_view->on_changed = [this](rf::Frequency f) {
			tx_frequency = f;
			if(!rx_enabled)
				transmitter_model.set_tuning_frequency(f);
			this->field_frequency.set_value(f);
			set_dirty();
		};
	};
	
	field_bw.on_change = [this](uint32_t v) {
		transmitter_model.set_channel_bandwidth(v * 1000);
	};
	field_bw.set_value(10);
	
	tx_gain = transmitter_model.tx_gain();
	field_rfgain.on_change = [this](int32_t v) {
		tx_gain = v;
		
	};
	field_rfgain.set_value(tx_gain);

	rf_amp = transmitter_model.rf_amp();
	field_rfamp.on_change = [this](int32_t v) {
		rf_amp = (bool)v;
	};
	field_rfamp.set_value(rf_amp ? 14 : 0);
	
	options_mode.on_change = [this](size_t, int32_t v) {
		enable_am = false;
		enable_usb = false;
		enable_lsb = false;
		enable_dsb = false;
		switch(v) {
			case 0:
				enable_am = false;
				enable_usb = false;
				enable_lsb = false;
				enable_dsb = false;
				field_bw.set_value(transmitter_model.channel_bandwidth() / 1000);
				//if (rx_enabled)
				rxaudio(rx_enabled); 					//Update now if we have RX audio on
				options_tone_key.hidden(0);				// we are in FM mode , we should have active the Key-tones & CTCSS option.
				field_rxbw.hidden(0);					// we are in FM mode,  we need to allow the user set up of the RX NFM BW selection (8K5, 11K, 16K) 
				break;
			case 1:
				enable_am = true;
				rxaudio(rx_enabled); 					//Update now if we have RX audio on
				options_tone_key.set_selected_index(0);	// we are NOT in FM mode ,  we reset the possible previous key-tones &CTCSS selection.
				set_dirty();							// Refresh display	
				options_tone_key.hidden(1);				// we hide that Key-tones & CTCSS input selecction, (no meaning in AM/DSB/SSB).
				field_rxbw.hidden(1);					// we hide the NFM BW selection in other modes non_FM  (no meaning in AM/DSB/SSB)
				check_rogerbeep.hidden(0);				// make visible again the "rogerbeep" selection.
				break;
			case 2:
				enable_usb = true;
				rxaudio(rx_enabled); 					//Update now if we have RX audio on
				check_rogerbeep.set_value(false);		// reset the possible activation of roger beep, because it is not compatible with SSB , by now. 
				check_rogerbeep.hidden(1);				// hide that roger beep selection. 
				set_dirty();							// Refresh display
				break;
			case 3:
				enable_lsb = true;
				rxaudio(rx_enabled); 					//Update now if we have RX audio on
				check_rogerbeep.set_value(false);		// reset the possible activation of roger beep, because it is not compatible with SSB , by now. 
				check_rogerbeep.hidden(1);				// hide that roger beep selection. 
				set_dirty();							// Refresh display
				break;
			case 4:
				enable_dsb = true;
				rxaudio(rx_enabled); 					//Update now if we have RX audio on
				check_rogerbeep.hidden(0);				// make visible again the "rogerbeep" selection.
				break;
		}
		//configure_baseband();
	};
	
	
	/*
	check_va.on_select = [this](Checkbox&, bool v) {
		va_enabled = v;
		text_ptt.hidden(v);			//hide / show PTT text
		check_rxactive.hidden(v); 	//hide / show the RX AUDIO
		set_dirty();				//Refresh display
	};
	*/
	field_va.set_selected_index(1);
	field_va.on_change = [this](size_t, int32_t v) {
		switch(v) {
			case 0:
				va_enabled = 0;
				this->set_ptt_visibility(0);
				check_rxactive.hidden(0);
				ptt_enabled = 0;
				break;
			case 1:
				va_enabled = 0;
				this->set_ptt_visibility(1);
				check_rxactive.hidden(0);
				ptt_enabled = 1;
				break;
			case 2:
				if (!rx_enabled) {
					va_enabled = 1;
					this->set_ptt_visibility(0);
					check_rxactive.hidden(1);
					ptt_enabled = 0;
				} else {
					field_va.set_selected_index(1);
				}
				break;
		}
		set_dirty();
	};
	

	check_rogerbeep.on_select = [this](Checkbox&, bool v) {
		rogerbeep_enabled = v;
	};

	field_va_level.on_change = [this](int32_t v) {
		va_level = v;
		vumeter.set_mark(v);
	};
	field_va_level.set_value(40);
	
	field_va_attack.on_change = [this](int32_t v) {
		attack_ms = v;
	};
	field_va_attack.set_value(500);
	
	field_va_decay.on_change = [this](int32_t v) {
		decay_ms = v;
	};
	field_va_decay.set_value(1000);

	check_rxactive.on_select = [this](Checkbox&, bool v) {
//		vumeter.set_value(0);	//Start with a clean vumeter
		rx_enabled = v;
//		check_va.hidden(v); 	//Hide or show voice activation
		rxaudio(v);				//Activate-Deactivate audio rx accordingly
		set_dirty();			//Refresh interface
	};

	field_volume.set_value((receiver_model.headphone_volume() - audio::headphone::volume_range().max).decibel() + 99);
	field_volume.on_change = [this](int32_t v) { this->on_headphone_volume_changed(v);	};

	field_rxbw.on_change = [this](size_t, int32_t v) {
		switch(v) {
			case 0:
				receiver_model.set_nbfm_configuration(0);
				break;
			case 1:
				receiver_model.set_nbfm_configuration(1);
				break;
			case 2:
				receiver_model.set_nbfm_configuration(2);
				break;
		}
	};
	field_rxbw.set_selected_index(2);

	field_squelch.on_change = [this](int32_t v) { 
		receiver_model.set_squelch_level(100 - v);	
	};
	field_squelch.set_value(0);
	receiver_model.set_squelch_level(0);

	rx_frequency = receiver_model.tuning_frequency();
	field_rxfrequency.set_value(rx_frequency);
	field_rxfrequency.set_step(receiver_model.frequency_step());
	field_rxfrequency.on_change = [this](rf::Frequency f) {
		rx_frequency = f;
		if(rx_enabled)
			receiver_model.set_tuning_frequency(f);
	};
	field_rxfrequency.on_edit = [this, &nav]() {
		focused_ui = 1;
		// TODO: Provide separate modal method/scheme?
		auto new_view = nav.push<FrequencyKeypadView>(rx_frequency);
		new_view->on_changed = [this](rf::Frequency f) {
			rx_frequency = f;
			if(rx_enabled)
				receiver_model.set_tuning_frequency(f);
			this->field_rxfrequency.set_value(f);
			set_dirty();
		};
	};

	
	rx_lna = receiver_model.lna();
	field_rxlna.on_change = [this](int32_t v) {
		rx_lna = v;
		if(rx_enabled)
			receiver_model.set_lna(v);
	};
	field_rxlna.set_value(rx_lna);

	rx_vga = receiver_model.vga();
	field_rxvga.on_change = [this](int32_t v) {
		rx_vga = v;
		if(rx_enabled)
			receiver_model.set_vga(v);
	};
	field_rxvga.set_value(rx_vga);

	rx_amp = receiver_model.rf_amp();
	field_rxamp.on_change = [this](int32_t v) {
		rx_amp = v;
		if(rx_enabled)
			receiver_model.set_rf_amp(rx_amp);
	};
	field_rxamp.set_value(rx_amp);

	tx_button.on_select = [this](Button&) {
		if(ptt_enabled && !transmitting) {
			set_tx(true);
		}
	};

	tx_button.on_touch_release = [this](Button&) {
		if(button_touch) {
			button_touch = false;
			set_tx(false);
		}
	};

	tx_button.on_touch_press = [this](Button&) {
		if(!transmitting) {
			button_touch = true;
		}	
	};

	transmitter_model.set_sampling_rate(sampling_rate);
	transmitter_model.set_baseband_bandwidth(1750000);
	
	set_tx(false);
	
	audio::set_rate(audio::Rate::Hz_24000);
	audio::input::start();		// originally , audio::input::start();  (we added parameter)
}

MicTXView::~MicTXView() {
	audio::input::stop();
	transmitter_model.set_tuning_frequency(tx_frequency); // Save Tx frequency instead of Rx. Or maybe we need some "System Wide" changes to seperate Tx and Rx frequency.
	transmitter_model.disable();
	if (rx_enabled) //Also turn off audio rx if enabled
		rxaudio(false);
	baseband::shutdown();
}

}
