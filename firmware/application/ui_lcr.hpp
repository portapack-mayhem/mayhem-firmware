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

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_textentry.hpp"
#include "message.hpp"
#include "transmitter_model.hpp"

namespace ui {

#define LCR_SCAN_COUNT 36

class LCRView : public View {
public:
	LCRView(NavigationView& nav);
	~LCRView();
	
	void focus() override;
	void paint(Painter& painter) override;
	
	std::string title() const override { return "LCR transmit"; };

private:
	enum tx_modes {
		IDLE = 0,
		SINGLE,
		SCAN
	};
	// afsk_config()
	tx_modes tx_mode = IDLE;
	bool abort_scan = false;
	double scan_progress;
	const char RGSB_list[LCR_SCAN_COUNT][5] = {
		"AI10",	"AI20", "AI30",	"AI40",
		"AI50",	"AI60", "AI70",	"AJ10",
		"AJ20",	"AJ30", "AJ40",	"AJ50",
		"AJ60",	"AJ70", "AK10",
		"EAA0", "EAB0",	"EAC0",	"EAD0",
		"EbA0",	"EbB0",	"EbC0",	"EbD0",
		"EbE0",	"EbF0",	"EbG0",	"EbH0",
		"EbI0",	"EbJ0",	"EbK0",	"EbL0",
		"EbM0",	"EbN0",	"EbO0",	"EbP0",
		"EbS0"
	};
	char litteral[5][8];
	char rgsb[5];
	char lcr_message[256];
	char lcr_string[256];			// For debugging, can remove
	char lcr_message_data[256];
	char checksum = 0;
	rf::Frequency f;
	uint8_t repeat_index;
	unsigned int scan_index;
	
	void generate_message();
	void update_progress();
	void start_tx(const bool scan);
	void on_txdone(int n);
	void on_button_setam(NavigationView& nav, Button& button);
	
	radio::Configuration lcr_radio_config = {
		0,
		2280000,	// ?
		2500000,	// ?
		rf::Direction::Transmit,
		true,
		0,
		0,
		1,
	};
	
	// 2: 94 ?
	// 9: 85 ?
	
	const char alt_lookup[128] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0F,	// 0
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,		// 10
		0xF8, 0, 0x99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,		// 20  !"#$%&'()*+,-./
		0xF5, 0, 0x94, 0x55, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1C, 0, 0,		// 30 0123456789:;<=>?
		0, 0x3C, 0x9C, 0x5D, 0, 0, 0, 0, 0, 0x44, 0x85, 0, 0xD5, 0x14, 0, 0,		// 40 @ABCDEFGHIJKLMNO
		0xF0, 0, 0, 0x50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,		// 50 PQRSTUVWXYZ[\]^_
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,		// 60 `abcdefghijklmno
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7F	// 70 pqrstuvwxyz{|}~
	};
	
	const Style style_val {
		.font = font::fixed_8x16,
		.background = Color::green(),
		.foreground = Color::black(),
	};
	const Style style_cancel {
		.font = font::fixed_8x16,
		.background = Color::red(),
		.foreground = Color::black(),
	};
	
	std::array<Button, 5> buttons;
	std::array<Checkbox, 5> checkboxes;

	Text text_recap {
		{ 8, 6, 18 * 8, 16 },
		"-"
	};
	
	OptionsField options_ec {
		{ 21 * 8, 6 },
		7,
		{
			{ "EC:Auto", 0 },
			{ "EC:Jour", 1 },
			{ "EC:Nuit", 2 }
		}
	};

	Button button_setrgsb {
		{ 16, 24, 96, 32 },
		"Set RGSB"
	};
	Button button_txsetup {
		{ 128, 24, 96, 32 },
		"TX setup"
	};
	
	Button button_lcrdebug {
		{ 168, 216, 56, 24 },
		"DEBUG"
	};
	
	Text text_status {
		{ 16, 224, 128, 16 },
		"Ready"
	};
	ProgressBar progress {
		{ 16, 224 + 20, 208, 16 }
	};
	
	Button button_transmit {
		{ 16, 270, 64, 32 },
		"TX"
	};
	Button button_scan {
		{ 88, 270, 64, 32 },
		"SCAN"
	};
	Button button_clear {
		{ 160, 270, 64, 32 },
		"CLEAR"
	};
	
	MessageHandlerRegistration message_handler_tx_done {
		Message::ID::TXDone,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const TXDoneMessage*>(p);
			this->on_txdone(message.n);
		}
	};
};

} /* namespace ui */
