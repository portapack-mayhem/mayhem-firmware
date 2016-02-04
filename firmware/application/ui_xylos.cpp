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

#include "ui_xylos.hpp"

#include "ch.h"
#include "hackrf_hal.hpp"

#include "event_m0.hpp"
#include "ui_alphanum.hpp"
#include "ff.h"
#include "hackrf_gpio.hpp"
#include "portapack.hpp"
#include "radio.hpp"

#include "hackrf_hal.hpp"
#include "portapack_shared_memory.hpp"
#include "portapack_persistent_memory.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

namespace ui {
	
void XylosRXView::talk() {
	uint8_t c;
	
	xylos_voice_phrase[0] = XYLOS_VOICE_HEADER;			// Header
	for (c=0; c<4; c++)
		xylos_voice_phrase[c+1] = XYLOS_VOICE_ZERO + ccir_received[c];
	xylos_voice_phrase[5] = XYLOS_VOICE_HEADER + 1;		// City
	xylos_voice_phrase[6] = XYLOS_VOICE_ZERO + ccir_received[4];
	xylos_voice_phrase[7] = XYLOS_VOICE_ZERO + ccir_received[5];
	xylos_voice_phrase[8] = XYLOS_VOICE_HEADER + 2;		// Family
	xylos_voice_phrase[9] = XYLOS_VOICE_ZERO + ccir_received[6];
	xylos_voice_phrase[10] = XYLOS_VOICE_HEADER + 3;	// Subfamily
	xylos_voice_phrase[11] = XYLOS_VOICE_ZERO + ccir_received[7];
	xylos_voice_phrase[12] = XYLOS_VOICE_HEADER + 4;	// Address
	xylos_voice_phrase[13] = XYLOS_VOICE_ZERO + ccir_received[8];
	xylos_voice_phrase[14] = XYLOS_VOICE_ZERO + ccir_received[9];
	xylos_voice_phrase[15] = XYLOS_VOICE_RELAYS;		// Relays
	for (c=0; c<4; c++) {
		xylos_voice_phrase[(c*2)+16] = XYLOS_VOICE_ZERO + 1 + c;
		xylos_voice_phrase[(c*2)+17] = XYLOS_VOICE_RELAYS + 1 + ccir_received[c+11];
	}
	xylos_voice_phrase[24] = XYLOS_VOICE_TRAILER;		// Trailer
	for (c=0; c<4; c++)
		xylos_voice_phrase[c+25] = XYLOS_VOICE_ZERO + ccir_received[c+16];
	xylos_voice_phrase[29] = 0xFF;
}

void XylosRXView::focus() {
	button_start.focus();
}

XylosRXView::~XylosRXView() {
	receiver_model.disable();
}

/*VirtualTimer vt;

void XylosRXView::do_something(void *p) {
	//p.set(xylos_voice_filenames[xylos_voice_phrase[p_idx]]);
	//p_idx++;
	//} while (xylos_voice_phrase[p_idx] != 0xFF);
	text_dbg.set("Done :)");
}*/

void XylosRXView::on_show() {
	//chVTSet(&vt, MS2ST(1000), do_something, NULL);
}

XylosRXView::XylosRXView(
	NavigationView& nav
)
{
	char ccirdebug[21] = { 0,0,0,0,1,8,1,10,10,10,11,1,1,2,0,11,0,0,0,0,0xFF };
	
	memcpy(ccir_received, ccirdebug, 21);
	
	add_children({ {
		&text_dbg,
		&button_start,
		&button_exit
	} });

	button_start.on_select = [this](Button&) {
		talk();
		p_idx = 0;
	};

	button_exit.on_select = [&nav](Button&){
		nav.pop();
	};
	
}




void XylosView::focus() {
	city_code.focus();
}

XylosView::~XylosView() {
	transmitter_model.disable();
}

void XylosView::paint(Painter& painter) {
	(void)painter;
}

void XylosView::upd_message() {
	uint8_t c;
		
	ccirmessage[0] = '0';
	ccirmessage[1] = '0';
	ccirmessage[2] = '0';
	ccirmessage[3] = '0';
	
	ccirmessage[4] = (city_code.value() / 10) + 0x30;
	ccirmessage[5] = (city_code.value() % 10) + 0x30;
	ccirmessage[6] = family_code.value() + 0x30;
	if (checkbox_wcsubfamily.value() == false)
		ccirmessage[7] = subfamily_code.value() + 0x30;
	else
		ccirmessage[7] = 'A';
	if (checkbox_wcid.value() == false) {
		ccirmessage[8] = (receiver_code.value() / 10) + 0x30;
		ccirmessage[9] = (receiver_code.value() % 10) + 0x30;
	} else {
		ccirmessage[8] = 'A';
		ccirmessage[9] = 'A';
	}
	
	ccirmessage[10] = 'B';
	
	ccirmessage[11] = options_ra.selected_index() + 0x30;
	ccirmessage[12] = options_rb.selected_index() + 0x30;
	ccirmessage[13] = options_rc.selected_index() + 0x30;
	ccirmessage[14] = '0';
	
	ccirmessage[15] = 'B';

	ccirmessage[16] = '0';
	ccirmessage[17] = '0';
	ccirmessage[18] = '0';
	ccirmessage[19] = '0';
	
	// Stuffing
	for (c=1; c<20; c++) {
		if (ccirmessage[c] == ccirmessage[c-1]) ccirmessage[c] = 'E';
	}
	
	ccirmessage[20] = 0;
	
	// Display as text
	text_debug.set(ccirmessage);
	
	// ASCII to frequency LUT index
	for (c=0; c<20; c++) {
		if (ccirmessage[c] > '9')
			ccirmessage[c] -= 0x37;
		else
			ccirmessage[c] -= 0x30;
	}
	
	ccirmessage[20] = 0xFF;
}

void XylosView::journuit() {
	uint8_t sr;
	
	chThdSleepMilliseconds(1000);
	
	// Invert relay states
	sr = options_ra.selected_index();
	if (sr > 0) options_ra.set_selected_index(sr ^ 3);
	sr = options_rb.selected_index();
	if (sr > 0) options_rb.set_selected_index(sr ^ 3);
	sr = options_rc.selected_index();
	if (sr > 0) options_rc.set_selected_index(sr ^ 3);
	
	upd_message();
	
	portapack::audio_codec.set_headphone_volume(volume_t::decibel(90 - 99) + wolfson::wm8731::headphone_gain_range.max);
	shared_memory.xylos_transmit_done = false;
	memcpy(shared_memory.xylosdata, ccirmessage, 21);
	transmitter_model.enable();
}

XylosView::XylosView(
	NavigationView& nav
)
{
	static constexpr Style style_val {
		.font = font::fixed_8x16,
		.background = Color::green(),
		.foreground = Color::black(),
	};
	
	static constexpr Style style_cancel {
		.font = font::fixed_8x16,
		.background = Color::red(),
		.foreground = Color::black(),
	};
	
	transmitter_model.set_baseband_configuration({
		.mode = 4,
		.sampling_rate = 1536000,
		.decimation_factor = 1,
	});

	add_children({ {
		&text_title,
		&button_txtest,
		&text_city,
		&city_code,
		&text_family,
		&family_code,
		&text_subfamily,
		&subfamily_code,
		&checkbox_wcsubfamily,
		&text_receiver,
		&receiver_code,
		&checkbox_wcid,
		&text_freq,
		&options_freq,
		&text_ra,
		&options_ra,
		&text_rb,
		&options_rb,
		&text_rc,
		&options_rc,
		&text_progress,
		&text_debug,
		&button_transmit,
		&checkbox_cligno,
		&button_exit
	} });
	
	city_code.set_value(18);
	family_code.set_value(1);
	subfamily_code.set_value(1);
	receiver_code.set_value(1);
	options_freq.set_selected_index(5);
	
	checkbox_wcsubfamily.set_value(true);
	checkbox_wcid.set_value(true);
	
	city_code.on_change = [this](int32_t v) {
		(void)v;
		XylosView::upd_message();
	};
	family_code.on_change = [this](int32_t v) {
		(void)v;
		XylosView::upd_message();
	};
	subfamily_code.on_change = [this](int32_t v) {
		(void)v;
		XylosView::upd_message();
	};
	receiver_code.on_change = [this](int32_t v) {
		(void)v;
		XylosView::upd_message();
	};
	checkbox_wcsubfamily.on_select = [this](Checkbox&) {
		if (checkbox_wcsubfamily.value() == true)
			subfamily_code.hidden(true);
		else
			subfamily_code.hidden(false);
		XylosView::upd_message();
	};
	checkbox_wcid.on_select = [this](Checkbox&) {
		if (checkbox_wcid.value() == true)
			receiver_code.hidden(true);
		else
			receiver_code.hidden(false);
		XylosView::upd_message();
	};
	options_ra.on_change = [this](size_t n, OptionsField::value_t v) {
		(void)n;
		(void)v;
		XylosView::upd_message();
	};
	options_rb.on_change = [this](size_t n, OptionsField::value_t v) {
		(void)n;
		(void)v;
		XylosView::upd_message();
	};
	options_rc.on_change = [this](size_t n, OptionsField::value_t v) {
		(void)n;
		(void)v;
		XylosView::upd_message();
	};
	
	button_transmit.set_style(&style_val);
	
	button_txtest.on_select = [this,&transmitter_model](Button&) {
		const uint8_t ccirtest[21] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,14,13,12,11,0xFF };
		if (txing == false) {
			EventDispatcher::message_map().unregister_handler(Message::ID::TXDone);
			
			EventDispatcher::message_map().register_handler(Message::ID::TXDone,
				[this,&transmitter_model](Message* const p) {
					const auto message = static_cast<const TXDoneMessage*>(p);
					if (message->n == 25) {
						portapack::audio_codec.set_headphone_volume(volume_t::decibel(0 - 99) + wolfson::wm8731::headphone_gain_range.max);
						transmitter_model.disable();
						txing = false;
						button_txtest.set_style(&style_val);
						button_txtest.set_text("TX TEST");
					}
				}
			);
			
			memcpy(ccirmessage, ccirtest, 21);
			shared_memory.xylos_transmit_done = false;
			memcpy(shared_memory.xylosdata, ccirmessage, 21);

			transmitter_model.set_tuning_frequency(xylos_freqs[options_freq.selected_index()]);
			
			portapack::audio_codec.set_headphone_volume(volume_t::decibel(90 - 99) + wolfson::wm8731::headphone_gain_range.max);

			txing = true;
			button_txtest.set_style(&style_cancel);
			button_txtest.set_text("Wait");
			transmitter_model.enable();
		}
	};

	button_transmit.on_select = [this,&transmitter_model](Button&) {
		if (txing == false) {
			upd_message();
			
			EventDispatcher::message_map().unregister_handler(Message::ID::TXDone);
			
			EventDispatcher::message_map().register_handler(Message::ID::TXDone,
				[this,&transmitter_model](Message* const p) {
					uint8_t c;
					char progress[21];
					const auto message = static_cast<const TXDoneMessage*>(p);
					if (message->n == 25) {
						portapack::audio_codec.set_headphone_volume(volume_t::decibel(0 - 99) + wolfson::wm8731::headphone_gain_range.max);
						transmitter_model.disable();
						for (c=0;c<20;c++)
							progress[c] = ' ';
						progress[20] = 0;
						text_progress.set(progress);
						if (checkbox_cligno.value() == false) {
							txing = false;
							button_transmit.set_style(&style_val);
							button_transmit.set_text("START");
						} else {
							journuit();
						}
					} else {
						for (c=0;c<message->n;c++)
							progress[c] = ' ';
						progress[c] = '.';
						progress[++c] = 0;
						text_progress.set(progress);
					}
				}
			);
			
			shared_memory.xylos_transmit_done = false;
			memcpy(shared_memory.xylosdata, ccirmessage, 21);

			transmitter_model.set_tuning_frequency(xylos_freqs[options_freq.selected_index()]);
			
			portapack::audio_codec.set_headphone_volume(volume_t::decibel(90 - 99) + wolfson::wm8731::headphone_gain_range.max);

			txing = true;
			button_transmit.set_style(&style_cancel);
			button_transmit.set_text("Wait");
			transmitter_model.enable();
		}
	};

	button_exit.on_select = [&nav](Button&){
		nav.pop();
	};
	
}

} /* namespace ui */
