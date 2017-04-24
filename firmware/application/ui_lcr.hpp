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

class LCRView : public View {
public:
	LCRView(NavigationView& nav);
	~LCRView();
	
	void focus() override;
	void paint(Painter& painter) override;
	
	std::string title() const override { return "LCR transmit"; };
	
private:
	struct scan_list_t {
		uint8_t count;
		const std::string * addresses;
	};
	
	const scan_list_t scan_list[2] = {
		{ 36, &RGSB_list_Lille[0] },
		{ 20, &RGSB_list_Reims[0] }
	};
	 
	const std::string RGSB_list_Lille[36] = {
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
	
	const std::string  RGSB_list_Reims[20] = {
		"AI10",	"AI20", "AI30",	"AI40",
		"AI50",	"AI60", "AI70",
		"AJ10", "AJ20",	"AJ30", "AJ40",
		"AJ50", "AJ60",	"AJ70",
		"AK10", "AK20", "AK50", "AK60",
		"AK70", 
		"AP10"
	};

	enum tx_modes {
		IDLE = 0,
		SINGLE,
		SCAN
	};
	
	tx_modes tx_mode = IDLE;
	uint8_t scan_count { 0 }, scan_index { 0 };
	uint32_t scan_progress { 0 };
	std::string litteral[5] { "       " };
	std::string rgsb { "    " };
	char lcr_message[512];
	uint16_t lcr_message_data[256];
	rf::Frequency f { 0 };
	uint8_t repeat_index { 0 };
	
	std::vector<std::string> parse_litterals();
	void update_progress();
	void start_tx(const bool scan);
	void on_txdone(int n);
	void on_button_setam(NavigationView& nav, Button& button);
	
	const Style style_val {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::green(),
	};
	const Style style_cancel {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::red(),
	};
	
	Labels labels {
		{ { 2 * 8, 4 }, "EC:", Color::light_grey() },
		{ { 88, 268 }, "Scan list:", Color::light_grey() }
	};
	
	std::array<Button, 5> buttons { };
	std::array<Checkbox, 5> checkboxes { };
	std::array<Rectangle, 5> rectangles { };

	Text text_recap {
		{ 12 * 8, 4, 18 * 8, 16 },
		"-"
	};
	
	OptionsField options_ec {
		{ 40, 4 },
		4,
		{
			{ "Auto", 0 },
			{ "Jour", 1 },
			{ "Nuit", 2 },
			{ "S ? ", 3 }
		}
	};

	Button button_setrgsb {
		{ 8, 24, 80, 32 },
		"RGSB"
	};
	Button button_txsetup {
		{ 13 * 8, 24, 128, 32 },
		"Modem setup"
	};
	
	Button button_clear {
		{ 174, 64, 58, 19 * 8 },
		"CLEAR"
	};
	
	Text text_status {
		{ 16, 222, 128, 16 },
		"Ready"
	};
	ProgressBar progress {
		{ 16, 242, 208, 16 }
	};
	
	Button button_transmit {
		{ 8, 270, 64, 32 },
		"TX"
	};
	
	OptionsField options_scanlist {
		{ 88, 284 },
		6,
		{
			{ "Lille ", 0 },
			{ "Reims ", 1 }
		}
	};
	
	Button button_scan {
		{ 166, 270, 64, 32 },
		"SCAN"
	};
	
	MessageHandlerRegistration message_handler_tx_done {
		Message::ID::TXDone,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const TXDoneMessage*>(p);
			this->on_txdone(message.progress);
		}
	};
};

} /* namespace ui */
