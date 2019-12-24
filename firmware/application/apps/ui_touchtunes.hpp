/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
 * Copyright (C) 2018 NotPike
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
// The format is actually very simple if it is rather seen as short and long gaps between pulses (as seen in many OOK remotes).
// The frames and data rate suspiciously match the NEC infrared protocol (http://www.sbprojects.com/knowledge/ir/nec.php) without
// the address complement. The exact data rate would be 1786 baud (560us/fragment).
// NotPike: The data rate should be 1786 but the remote was transmitting at 1766 

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

#define TOUCHTUNES_MAX_PIN 255
#define TOUCHTUNES_REPEATS 4
#define TOUCHTUNES_SYNC_WORD 0x5D

// Each 16bit button code is actually 8bit followed by its complement
const uint8_t button_codes[32] = {
	0x32,	// Pause
	0x78,	// On/Off
	0x70,	// P1
	0x60,	// P2
	0xCA,	// P3
	0x20,	// F1
	0xF2,	// Up
	0xA0,	// F2
	0x84,	// Left
	0x44,	// OK
	0xC4,	// Right
	0x30,	// F3
	0x80,	// Down
	0xB0,	// F4
	0xF0,	// 1
	0x08,	// 2
	0x88,	// 3
	0x48,	// 4
	0xC8,	// 5
	0x28,	// 6
	0xA8,	// 7
	0x68,	// 8
	0xE8,	// 9
	0x18,	// Music_Karaoke
	0x98,	// 0
	0x58,	// Lock_Queue
	0xD0,	// Zone 1 Vol+
	0x90,	// Zone 2 Vol+
	0xC0,	// Zone 3 Vol+
	0x50,	// Zone 1 Vol-
	0x10,	// Zone 2 Vol-
	0x40,	// Zone 3 Vol-
};

namespace ui {

class TouchTunesView : public View {
public:
	TouchTunesView(NavigationView& nav);
	~TouchTunesView();
	
	void focus() override;
	
	std::string title() const override { return "TouchTunes TX"; };

private:
	uint32_t scan_button_index { };
	uint32_t pin { 0 };
	
	enum tx_modes {
		IDLE = 0,
		SINGLE,
		SCAN
	};
	
	tx_modes tx_mode = IDLE;
	
	void start_tx(const uint32_t button_index);
	void stop_tx();
	void on_tx_progress(const uint32_t progress, const bool done);

	struct remote_layout_t {
		Point position;
		std::string text;
	};
	
	const std::array<remote_layout_t, 32> remote_layout { {
		{ { 12 * 8, 0 }, "PAUSE" },
		{ { 21 * 8, 0 }, "POWER" },
		
		{ { 14 * 8, 5 * 8 }, "P1" },
		{ { 18 * 8, 5 * 8 }, "P2" },
		{ { 22 * 8, 5 * 8 }, "P3" },
		
		{ { 14 * 8, 10 * 8 }, "F1" },
		{ { 18 * 8 + 4, 10 * 8 }, "^" },
		{ { 22 * 8, 10 * 8 }, "F2" },
		
		{ { 14 * 8, 14 * 8 }, "<" },
		{ { 18 * 8, 14 * 8 }, "OK" },
		{ { 23 * 8, 14 * 8 }, ">" },
		
		{ { 14 * 8, 18 * 8 }, "F3" },
		{ { 18 * 8 + 4, 18 * 8 }, "V" },
		{ { 22 * 8, 18 * 8 }, "F4" },
		
		{ { 0 * 8, 5 * 8 }, "1" },
		{ { 4 * 8, 5 * 8 }, "2" },
		{ { 8 * 8, 5 * 8 }, "3" },
		{ { 0 * 8, 10 * 8 }, "4" },
		{ { 4 * 8, 10 * 8 }, "5" },
		{ { 8 * 8, 10 * 8 }, "6" },
		{ { 0 * 8, 15 * 8 }, "7" },
		{ { 4 * 8, 15 * 8 }, "8" },
		{ { 8 * 8, 15 * 8 }, "9" },
		{ { 0 * 8, 20 * 8 }, "*" },
		{ { 4 * 8, 20 * 8 }, "0" },
		{ { 8 * 8, 20 * 8 }, "#" },
		
		{ { 13 * 8, 23 * 8 }, "+" },
		{ { 18 * 8, 23 * 8 }, "+" },
		{ { 23 * 8, 23 * 8 }, "+" },
		
		{ { 13 * 8, 29 * 8 }, "-" },
		{ { 18 * 8, 29 * 8 }, "-" },
		{ { 23 * 8, 29 * 8 }, "-" }
	} };
	
	Labels labels {
		{ { 2 * 8, 1 * 8 }, "PIN:", Color::light_grey() },
		{ { 13 * 8 + 4, 27 * 8 }, "VOL1 VOL2 VOL3", Color::light_grey() }
	};
	
	std::array<Button, 32> buttons { };

	NumberField field_pin {
		{ 6 * 8, 1 * 8 },
		3,
		{ 0, 255 },
		1,
		'0'
	};
	
	Checkbox check_scan {
		{ 2 * 8, 27 * 8 },
		4,
		"Scan"
	};

	Text text_status {
		{ 2 * 8, 33 * 8, 128, 16 },
		"Ready"
	};
	
	ProgressBar progressbar {
		{ 2 * 8, 35 * 8, 208, 16 }
	};
	
	MessageHandlerRegistration message_handler_tx_progress {
		Message::ID::TXProgress,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
			this->on_tx_progress(message.progress, message.done);
		}
	};
};

} /* namespace ui */
