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
#include "ui_transmitter.hpp"

#include "message.hpp"
#include "transmitter_model.hpp"

namespace ui {
	
#define LCR_MAX_AM 5

class LCRView : public View {
public:
	LCRView(NavigationView& nav);
	~LCRView();
	
	void focus() override;
	
	std::string title() const override { return "TEDI/LCR TX"; };
	
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
	std::array<std::string, LCR_MAX_AM> litteral { { "       " } };
	std::string rgsb { "AI10" };
	uint16_t lcr_message_data[256];
	uint8_t repeat_index { 0 };
	
	void update_progress();
	void start_tx(const bool scan);
	void on_tx_progress(const uint32_t progress, const bool done);
	void on_button_set_am(NavigationView& nav, int16_t button_id);
	
	Labels labels {
		{ { 0, 8 }, "EC:     RGSB:", Color::light_grey() },
		{ { 17 * 8, 4 * 8 }, "List:", Color::light_grey() }
	};
	
	std::array<Button, LCR_MAX_AM> buttons { };
	std::array<Checkbox, LCR_MAX_AM> checkboxes { };
	std::array<Rectangle, LCR_MAX_AM> rectangles { };
	std::array<Text, LCR_MAX_AM> texts { };
	
	OptionsField options_ec {
		{ 3 * 8, 8 },
		4,
		{
			{ "Auto", 0 },
			{ "Jour", 1 },
			{ "Nuit", 2 },
			{ "S ? ", 3 }
		}
	};

	Button button_set_rgsb {
		{ 13 * 8, 4, 8 * 8, 24 },
		"RGSB"
	};
	Checkbox check_scan {
		{ 22 * 8, 4 },
		4,
		"Scan"
	};
	
	Button button_modem_setup {
		{ 1 * 8, 4 * 8 + 2, 14 * 8, 24 },
		"Modem setup"
	};
	OptionsField options_scanlist {
		{ 22 * 8, 4 * 8 },
		6,
		{
			{ "Reims ", 1 }
		}
	};
	
	Button button_clear {
		{ 22 * 8, 8 * 8, 7 * 8, 19 * 8 },
		"CLEAR"
	};
	
	Text text_status {
		{ 2 * 8, 27 * 8 + 4, 26 * 8, 16 },
		"Ready"
	};
	ProgressBar progress {
		{ 2 * 8, 29 * 8 + 4, 26 * 8, 16 }
	};
	
	TransmitterView tx_view {
		16 * 16,
		10000,
		12
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
