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
#include "ui_alphanum.hpp"

#include "portapack.hpp"
#include "baseband_api.hpp"
//#include "audio.hpp"
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
	baseband::shutdown();
}

void XylosView::generate_message() {
	uint8_t c;
	
	// Init CCIR message
	memcpy(ccir_message, ccir_base, 21);
	
	// Header
	ccir_message[0] = (header_code_a.value() / 10) + 0x30;
	ccir_message[1] = (header_code_a.value() % 10) + 0x30;
	ccir_message[2] = (header_code_b.value() / 10) + 0x30;
	ccir_message[3] = (header_code_b.value() % 10) + 0x30;
	
	// Addresses
	ccir_message[4] = (city_code.value() / 10) + 0x30;
	ccir_message[5] = (city_code.value() % 10) + 0x30;
	ccir_message[6] = family_code.value() + 0x30;
	
	if (checkbox_wcsubfamily.value() == false)
		ccir_message[7] = subfamily_code.value() + 0x30;
	else
		ccir_message[7] = 'A';
	
	if (checkbox_wcid.value() == false) {
		ccir_message[8] = (receiver_code.value() / 10) + 0x30;
		ccir_message[9] = (receiver_code.value() % 10) + 0x30;
	} else {
		ccir_message[8] = 'A';
		ccir_message[9] = 'A';
	}
	
	// Commands
	ccir_message[11] = options_ra.selected_index() + 0x30;
	ccir_message[12] = options_rb.selected_index() + 0x30;
	ccir_message[13] = options_rc.selected_index() + 0x30;
	ccir_message[14] = options_rd.selected_index() + 0x30;
	
	// Get rid of repeats with E code
	for (c = 1; c < 20; c++)
		if (ccir_message[c] == ccir_message[c - 1]) ccir_message[c] = 'E';
	
	// Display as text
	text_message.set(ccir_message);
	
	ascii_to_ccir(ccir_message);
}

void XylosView::start_tx() {
	//audio::headphone::set_volume(volume_t::decibel(90 - 99) + audio::headphone::volume_range().max);

	transmitter_model.set_tuning_frequency(xylos_freqs[options_freq.selected_index()]);
	transmitter_model.set_baseband_configuration({
		.mode = 0,
		.sampling_rate = 1536000U,
		.decimation_factor = 1,
	});
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_lna(40);
	transmitter_model.set_vga(40);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	memcpy(shared_memory.tx_data, ccir_message, 21);
	baseband::set_ccir_data(CCIR_TONELENGTH, 20);
}

	// ASCII to frequency LUT index
void XylosView::ascii_to_ccir(char *ascii) {
	uint8_t c;
	
	for (c = 0; c < 20; c++) {
		if (ascii[c] > '9')
			ascii[c] -= 0x37;
		else
			ascii[c] -= 0x30;
	}
	
	// EOM code for baseband
	ascii[20] = 0xFF;
}

void XylosView::on_txdone(const int n) {
	size_t sr;
	
	if (tx_mode == TESTING) {
		if (n == 25) {
			transmitter_model.disable();
			
			/*if (sequence_idx != 9) {
				chThdSleepMilliseconds(15000);
				memcpy(ccir_message, &xylos_sequence[sequence_idx][0], 21);
				// ASCII to frequency LUT index
				for (c=0; c<20; c++) {
					if (ccir_message[c] > '9')
						ccir_message[c] -= 0x37;
					else
						ccir_message[c] -= 0x30;
				}
				ccir_message[20] = 0xFF;
				//memcpy(shared_memory.xylosdata, ccirmessage, 21);					TODO !!!
				
				sequence_idx++;
				tx_mode = TESTING;
				transmitter_model.enable();
			} else {*/
				button_txtest.set_style(&style());
				button_txtest.set_text("TEST");
				tx_mode = IDLE;
			//}
		}
	} else {
		if (n == 25) {
			//audio::headphone::set_volume(volume_t::decibel(0 - 99) + audio::headphone::volume_range().max);
			transmitter_model.disable();
			progress.set_value(0);
			
			if (checkbox_cligno.value() == false) {
				tx_mode = IDLE;
				button_transmit.set_style(&style_val);
				button_transmit.set_text("START");
			} else {
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
				
				generate_message();
				start_tx();
			}
		} else {
			progress.set_value(n);
		}
	}
}

XylosView::XylosView(NavigationView& nav) {
	(void)nav;
	
	baseband::run_image(portapack::spi_flash::image_tag_xylos);

	add_children({ {
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
	
	progress.set_max(20);
	
	options_ra.set_selected_index(1);		// R1 OFF
	
	checkbox_wcsubfamily.set_value(true);
	checkbox_wcid.set_value(true);
	
	header_code_a.on_change = [this](int32_t v) {
		(void)v;
		generate_message();
	};
	header_code_b.on_change = [this](int32_t v) {
		(void)v;
		generate_message();
	};
	city_code.on_change = [this](int32_t v) {
		(void)v;
		generate_message();
	};
	family_code.on_change = [this](int32_t v) {
		(void)v;
		generate_message();
	};
	subfamily_code.on_change = [this](int32_t v) {
		(void)v;
		generate_message();
	};
	receiver_code.on_change = [this](int32_t v) {
		(void)v;
		generate_message();
	};
	
	checkbox_wcsubfamily.on_select = [this](Checkbox&) {
		if (checkbox_wcsubfamily.value() == true) {
			receiver_code.set_focusable(false);
			text_subfamily.set_style(&style_grey);
		} else {
			receiver_code.set_focusable(true);
			text_subfamily.set_style(&style());
		}
		generate_message();
	};
	
	checkbox_wcid.on_select = [this](Checkbox&) {
		if (checkbox_wcid.value() == true) {
			receiver_code.set_focusable(false);
			text_receiver.set_style(&style_grey);
		} else {
			receiver_code.set_focusable(true);
			text_receiver.set_style(&style());
		}
		receiver_code.set_dirty();
		generate_message();
	};
	
	options_ra.on_change = [this](size_t n, OptionsField::value_t v) {
		(void)n;
		(void)v;
		generate_message();
	};
	options_rb.on_change = [this](size_t n, OptionsField::value_t v) {
		(void)n;
		(void)v;
		generate_message();
	};
	options_rc.on_change = [this](size_t n, OptionsField::value_t v) {
		(void)n;
		(void)v;
		generate_message();
	};
	options_rd.on_change = [this](size_t n, OptionsField::value_t v) {
		(void)n;
		(void)v;
		generate_message();
	};
	
	button_transmit.set_style(&style_val);
	
	generate_message();
	
	// Transmission and tones testing
	button_txtest.on_select = [this](Button&) {
		// Tones going up in pitch
		const uint8_t ccir_test[21] = { 11, 13, 15, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 14, 12, 10, 12, 14, 0, 9, 0xFF };
		
		if (tx_mode == IDLE) {
			tx_mode = TESTING;
			memcpy(ccir_message, ccir_test, 21);
			//audio::headphone::set_volume(volume_t::decibel(90 - 99) + audio::headphone::volume_range().max);
			button_txtest.set_style(&style_cancel);
			button_txtest.set_text("Wait");
			start_tx();
		}
	};
	
	// Sequence playback
	/*button_txtest.on_select = [this](Button&) {
		if (tx_mode == IDLE) {
			tx_mode = TESTING;
			sequence_idx = 0;
			
			memcpy(ccir_message, &xylos_sequence[sequence_idx][0], 21);
			
			ascii_to_ccir(ccir_message);
			
			sequence_idx++;

			button_txtest.set_style(&style_cancel);
			button_txtest.set_text("Wait");
			start_tx();
		}
	};*/

	// Single transmit
	button_transmit.on_select = [this,&nav](Button&) {
		if (tx_mode == IDLE) {
			/*auto modal_view = nav.push<ModalMessageView>("TX", "TX ?", true);
			modal_view->on_choice = [this](bool choice) {
				if (choice) {*/
					// audio::headphone::set_volume(volume_t::decibel(90 - 99) + audio::headphone::volume_range().max);
					tx_mode = SINGLE;
					button_transmit.set_style(&style_cancel);
					button_transmit.set_text("Wait");
					generate_message();
					start_tx();
				//}
			//};
		}
	};
}

} /* namespace ui */
