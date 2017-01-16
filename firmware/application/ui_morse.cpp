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

/*
Keying speed: 60 or 75 PARIS
Continuous (Fox-oring)
12s transmit, 48s space (Sprint 1/5th) 
60s transmit, 240s space (Classic 1/5 min) 
60s transmit, 360s space (Classic 1/7 min) 
*/

#include "ui_morse.hpp"

#include "portapack.hpp"
#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

// TODO: Live keying mode: Dit on left key, dah on right ?

namespace ui {

void MorseView::focus() {
	button_transmit.focus();
}

MorseView::~MorseView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void MorseView::paint(Painter& painter) {
	(void)painter;
}

void MorseView::generate_message(char * text) {
	char ch;
	size_t i, c;
	uint8_t code;
	uint8_t morse_message[256];
	
	ToneDef * tone_defs = shared_memory.bb_data.tones_data.tone_defs;
	
	// TODO: OOB check on morse_message[]
	
	i = 0;
	while ((ch = (*text++))) {
		if ((ch >= 'a') && (ch <= 'z'))		// Make uppercase
			ch -= 32;
		
		if ((ch >= 'A') && (ch <= 'Z')) {
			code = morse_ITU[ch - 'A' + 10];
			for (c = 0; c < (code & 7); c++) {
				morse_message[i++] = (code << c) >> 7;	// Dot/dash
				morse_message[i++] = 2;					// Silence
			}
			morse_message[i - 1] = 3;					// Letter silence
		} else if (ch == ' ') {
			morse_message[i++] = 4;						// Word silence
		}
			
	}
	
	memcpy(shared_memory.bb_data.tones_data.message, morse_message, i);
	
	(*tone_defs).delta = MORSE_TONE_DELTA;	// Dot
	(*tone_defs++).duration = MORSE_DOT;
	(*tone_defs).delta = MORSE_TONE_DELTA;	// Dash
	(*tone_defs++).duration = MORSE_DASH;
	(*tone_defs).delta = 0;					// 1 unit silence
	(*tone_defs++).duration = MORSE_SPACE;
	(*tone_defs).delta = 0;					// 3 unit silence
	(*tone_defs++).duration = MORSE_LETTER_SPACE;
	(*tone_defs).delta = 0;					// 7 unit silence
	(*tone_defs++).duration = MORSE_WORD_SPACE;
	
	transmitter_model.set_tuning_frequency(81800000);
	transmitter_model.set_sampling_rate(1536000U);
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_lna(40);
	transmitter_model.set_vga(40);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	//audio::set_rate(audio::Rate::Hz_24000);
	baseband::set_tones_data(5000, 0, i, false, false);
}

void MorseView::transmit_done() {
	transmitter_model.disable();
}

MorseView::MorseView(
	NavigationView& nav
)
{
	add_children({
		&checkbox_foxhunt,
		&options_foxhunt,
		&button_transmit,
		&button_exit
	});
	
	button_transmit.on_select = [this](Button&){
		//char strtest[] = "TEST";
		//generate_message(strtest);
	};

	button_exit.on_select = [&nav](Button&){
		nav.pop();
	};

}

} /* namespace ui */
