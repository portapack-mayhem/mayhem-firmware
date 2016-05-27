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
#include "audio.hpp"
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

/*
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
*/



void XylosView::focus() {
	options_ra.focus();
}

XylosView::~XylosView() {
	transmitter_model.disable();
}

void XylosView::paint(Painter& painter) {
	(void)painter;
}

void XylosView::upd_message() {
	uint8_t c;
		
	ccirmessage[0] = (header_code_a.value() / 10) + 0x30;
	ccirmessage[1] = (header_code_a.value() % 10) + 0x30;
	ccirmessage[2] = (header_code_b.value() / 10) + 0x30;
	ccirmessage[3] = (header_code_b.value() % 10) + 0x30;
	
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
	ccirmessage[14] = options_rd.selected_index() + 0x30;
	
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
	text_message.set(ccirmessage);
	
	// ASCII to frequency LUT index
	for (c=0; c<20; c++) {
		if (ccirmessage[c] > '9')
			ccirmessage[c] -= 0x37;
		else
			ccirmessage[c] -= 0x30;
	}
	
	ccirmessage[20] = 0xFF;
}

void XylosView::start_tx() {
	upd_message();
	
	audio::headphone::set_volume(volume_t::decibel(90 - 99) + audio::headphone::volume_range().max);
	shared_memory.transmit_done = false;
	memcpy(shared_memory.xylosdata, ccirmessage, 21);
	transmitter_model.enable();
}

XylosView::XylosView(
	NavigationView& nav
)
{
	transmitter_model.set_baseband_configuration({
		.mode = 2,
		.sampling_rate = 1536000,
		.decimation_factor = 1,
	});

	add_children({ {
		&checkbox_hinc,
		&button_txtest,
		&text_header,
		&header_code_a,
		&header_code_b,
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
		&text_relais,
		&options_ra,
		&options_rb,
		&options_rc,
		&options_rd,
		&progress,
		&text_message,
		&button_transmit,
		&checkbox_cligno,
		&tempo_cligno,
		&text_cligno
	} });
	
	city_code.set_value(18);
	family_code.set_value(1);
	subfamily_code.set_value(1);
	receiver_code.set_value(1);
	header_code_a.set_value(0);
	header_code_b.set_value(0);
	options_freq.set_selected_index(5);
	tempo_cligno.set_value(5);
	
	options_ra.set_selected_index(1);		// R1 OFF
	
	checkbox_wcsubfamily.set_value(true);
	checkbox_wcid.set_value(true);
	
	header_code_a.on_change = [this](int32_t v) {
		(void)v;
		XylosView::upd_message();
	};
	header_code_b.on_change = [this](int32_t v) {
		(void)v;
		XylosView::upd_message();
	};
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
	
	subfamily_code.hidden(true);
	text_subfamily.set_style(&style_grey);
	checkbox_wcsubfamily.on_select = [this](Checkbox&) {
		if (checkbox_wcsubfamily.value() == true) {
			subfamily_code.hidden(true);
			text_subfamily.set_style(&style_grey);
		} else {
			subfamily_code.hidden(false);
			text_subfamily.set_style(&style());
		}
		XylosView::upd_message();
	};
	
	receiver_code.hidden(true);
	text_receiver.set_style(&style_grey);
	checkbox_wcid.on_select = [this](Checkbox&) {
		if (checkbox_wcid.value() == true) {
			receiver_code.hidden(true);
			text_receiver.set_style(&style_grey);
		} else {
			receiver_code.hidden(false);
			text_receiver.set_style(&style());
		}
		receiver_code.set_dirty();
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
	options_rd.on_change = [this](size_t n, OptionsField::value_t v) {
		(void)n;
		(void)v;
		XylosView::upd_message();
	};
	
	button_transmit.set_style(&style_val);
	
	XylosView::upd_message();
	
	button_txtest.on_select = [this](Button&) {
		const uint8_t ccirtest[21] = { 11, 13, 15, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 14, 12, 10, 12, 14, 0, 9, 0xFF };
		if (txing == false) {
			EventDispatcher::message_map().unregister_handler(Message::ID::TXDone);
			
			EventDispatcher::message_map().register_handler(Message::ID::TXDone,
				[this](Message* const p) {
					const auto message = static_cast<const TXDoneMessage*>(p);
					if (message->n == 25) {
						audio::headphone::set_volume(volume_t::decibel(0 - 99) + audio::headphone::volume_range().max);
						transmitter_model.disable();
						txing = false;
						button_txtest.set_style(&style());
						button_txtest.set_text("TEST");
					}
				}
			);
			
			memcpy(ccirmessage, ccirtest, 21);
			shared_memory.transmit_done = false;
			memcpy(shared_memory.xylosdata, ccirmessage, 21);

			transmitter_model.set_tuning_frequency(xylos_freqs[options_freq.selected_index()]);
			
			audio::headphone::set_volume(volume_t::decibel(90 - 99) + audio::headphone::volume_range().max);

			txing = true;
			button_txtest.set_style(&style_cancel);
			button_txtest.set_text("Wait");
			transmitter_model.enable();
		}
	};

	button_transmit.on_select = [this](Button&) {
		
		if (txing == false) {
			upd_message();
			
			if (checkbox_hinc.value())
				inc_cnt = 3;
			else
				inc_cnt = 0;
			header_init = header_code_b.value();
			
			EventDispatcher::message_map().unregister_handler(Message::ID::TXDone);
			
			EventDispatcher::message_map().register_handler(Message::ID::TXDone,
				[this](Message* const p) {
					uint8_t sr;
					const auto message = static_cast<const TXDoneMessage*>(p);
					if (message->n == 25) {
						audio::headphone::set_volume(volume_t::decibel(0 - 99) + audio::headphone::volume_range().max);
						transmitter_model.disable();
						progress.set_value(0);
						
						if (inc_cnt) {
							chThdSleepMilliseconds(1000);
							header_code_b.set_value(header_code_b.value() + 1);
							
							upd_message();
							start_tx();
							
							inc_cnt--;
						} else {
							header_code_b.set_value(header_init);
							if (checkbox_cligno.value() == false) {
								txing = false;
								button_transmit.set_style(&style_val);
								button_transmit.set_text("START");
							} else {
								if (checkbox_hinc.value())
									inc_cnt = 3;
								else
									inc_cnt = 0;
								
								chThdSleepMilliseconds(tempo_cligno.value() * 1000);
								
								// Invert relay states
								sr = options_ra.selected_index();
								if (sr > 0) options_ra.set_selected_index(sr ^ 3);
								sr = options_rb.selected_index();
								if (sr > 0) options_rb.set_selected_index(sr ^ 3);
								sr = options_rc.selected_index();
								if (sr > 0) options_rc.set_selected_index(sr ^ 3);
								sr = options_rd.selected_index();
								if (sr > 0) options_rd.set_selected_index(sr ^ 3);
								
								upd_message();
								start_tx();
							}
						}
					} else {
						progress.set_value((message->n + 1) * 5);
					}
				}
			);
			
			shared_memory.transmit_done = false;
			memcpy(shared_memory.xylosdata, ccirmessage, 21);

			transmitter_model.set_tuning_frequency(xylos_freqs[options_freq.selected_index()]);
			
			audio::headphone::set_volume(volume_t::decibel(90 - 99) + audio::headphone::volume_range().max);

			txing = true;
			button_transmit.set_style(&style_cancel);
			button_transmit.set_text("Wait");
			transmitter_model.enable();
		}
	};
	
}

} /* namespace ui */
