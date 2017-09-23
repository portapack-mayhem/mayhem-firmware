/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#include "ui.hpp"
#include "ui_transmitter.hpp"
#include "transmitter_model.hpp"

// The coding in notpike's script is quite complex, using multiple LUTs to form the data sent to the YSO.
// The format is actually very simple if it is rather seen as short and long gaps between pulses (as seen in many OOK remotes):

// Pin 0 - On/Off
// ffff00 a2888a2aaaa8888aa2aa2220
// 1010 0010 1000 1000 1000 1010 0010 1010 1010 1010 1010 1000 1000 1000 1000 1010 1010 0010 1010 1010 0010 0010 0010 0000
// 101000101000100010001010001010101010101010101000100010001000101010100010101010100010001000100000
// S L   S L   L   L   S L   S S S S S S S S S L   L   L   L   S S S L   S S S S L   L   L

// Pin 1 - On/Off
// ffff00 a2888a22aaaa2222a8aa8888
// 1010 0010 1000 1000 1000 1010 0010 0010 1010 1010 1010 1010 0010 0010 0010 0010 1010 1000 1010 1010 1000 1000 1000 1000
// 101000101000100010001010001000101010101010101010001000100010001010101000101010101000100010001000
// S L   S L   L   L   S L   L   S S S S S S S S L   L   L   L   S S S L   S S S S L   L   L

// Pin 2 - On/Off
// ffff00 a2888a28aaaa2222a8aa8888
// 1010 0010 1000 1000 1000 1010 0010 1000 1010 1010 1010 1010 0010 0010 0010 0010 1010 1000 1010 1010 1000 1000 1000 1000
// 101000101000100010001010001010001010101010101010001000100010001010101000101010101000100010001000
// S L   S L   L   L   S L   S L   S S S S S S S L   L   L   L   S S S L   S S S S L   L   L

// Pin 3 - On/Off
// ffff00 a2888a222aaa8888aa2aa222
// 1010 0010 1000 1000 1000 1010 0010 0010 0010 1010 1010 1010 1000 1000 1000 1000 1010 1010 0010 1010 1010 0010 0010 0010 0000 0000
// 10100010100010001000101000100010001010101010101010001000100010001010101000101010101000100010001000
// S L   S L   L   L   S L   L   L   S S S S S S S L   L   L   L   S S S L   S S S S L   L   L

// The sync word seems to be SLSLLLSL (01011101, or 0xBA reversed)
// The pin # is sent LSB first
// The button data seems to be SLLLLSSSLSSSSLLL (0111100010000111 and a terminating pulse, or 0x7887 reversed)
// The hex data only seems scrambled because of the shift induced by the short or long gaps (10 or 1000)
// The radio frame's duration depends on the value of the bits

const uint16_t button_codes[7] = {
	0x7887,	// On/Off
	0xB34C,	// Pause
	0xF10E,	// P1
	0xDD22,	// OK
	0xF40B,	// Zone 1 Vol+
	0xF609,	// Zone 2 Vol+
	0xFC03	// Zone 3 Vol+
};

namespace ui {

class TouchTunesView : public View {
public:
	TouchTunesView(NavigationView& nav);
	~TouchTunesView();
	
	void focus() override;
	
	std::string title() const override { return "TouchTunes TX"; };

private:
	Labels labels {
		{ { 2 * 8, 2 * 8 }, "PIN:", Color::light_grey() }
	};
	
	uint32_t scan_button_index { };
	uint32_t pin { 0 };
	
	enum tx_modes {
		IDLE = 0,
		SINGLE,
		SCAN
	};
	
	tx_modes tx_mode = IDLE;
	
	void start_tx(const uint32_t button_index);
	void on_tx_progress(const int progress, const bool done);

	NumberField field_pin {
		{ 6 * 8, 1 * 16 },
		3,
		{ 0, 255 },
		1,
		' '
	};
	
	Button button_on_off {
		{ 2 * 8, 3 * 16, 8 * 8, 2 * 16 },
		"ON/OFF"
	};
	
	Button button_pause {
		{ 2 * 8, 6 * 16, 8 * 8, 2 * 16 },
		"PAUSE"
	};
	Button button_p1 {
		{ 11 * 8, 6 * 16, 8 * 8, 2 * 16 },
		"P1"
	};
	Button button_ok {
		{ 20 * 8, 6 * 16, 8 * 8, 2 * 16 },
		"OK"
	};
	
	Button button_vol_inc1 {
		{ 2 * 8, 9 * 16, 8 * 8, 2 * 16 },
		"VOL+ 1"
	};
	Button button_vol_inc2 {
		{ 11 * 8, 9 * 16, 8 * 8, 2 * 16 },
		"VOL+ 2"
	};
	Button button_vol_inc3 {
		{ 20 * 8, 9 * 16, 8 * 8, 2 * 16 },
		"VOL+ 3"
	};
	
	Checkbox check_scan {
		{ 2 * 8, 13 * 16 },
		4,
		"Scan"
	};

	Text text_status {
		{ 2 * 8, 15 * 16, 128, 16 },
		"Ready"
	};
	
	ProgressBar progressbar {
		{ 2 * 8, 15 * 16 + 20, 208, 16 }
	};
	
	MessageHandlerRegistration message_handler_tx_done {
		Message::ID::TXDone,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const TXDoneMessage*>(p);
			this->on_tx_progress(message.progress, message.done);
		}
	};
};

} /* namespace ui */
