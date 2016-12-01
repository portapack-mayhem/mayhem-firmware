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
#include "ui_textentry.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_font_fixed_8x16.hpp"

#include "message.hpp"
#include "transmitter_model.hpp"
#include "portapack.hpp"

namespace ui {

class ADSBTxView : public View {
public:
	ADSBTxView(NavigationView& nav);
	~ADSBTxView();
	
	void paint(Painter& painter) override;
	
	void focus() override;
	
	std::string title() const override { return "ADS-B transmit"; };

private:
	enum tx_modes {
		IDLE = 0,
		SINGLE,
		SEQUENCE
	};
	
	tx_modes tx_mode = IDLE;
	
	char callsign[9] = "KLM1023 ";
	
	uint8_t adsb_frame[14];		// 112 bit data block as 14 bytes
	uint8_t adsb_bin[112];		// 112 bit data block
	
	const char icao_id_lut[65] = "#ABCDEFGHIJKLMNOPQRSTUVWXYZ##### ###############0123456789######";
	
	void start_tx();
	void generate_frame_id();
	void generate_frame_pos();
	void on_txdone(const int n);
	
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
	const Style style_grey {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::grey(),
	};
	
	Text text_format {
		{ 4 * 8, 1 * 16, 7 * 8, 16 },
		"Format:"
	};
	OptionsField options_format {
		{ 12 * 8, 1 * 16 },
		10,
		{
			{ "17: ADS-B", 17 },
			{ "18: TIS-B", 18 },
			{ "19: Don't.", 19 },
		}
	};
	
	Text text_icaolabel {
		{ 4 * 8, 3 * 16, 7 * 8, 16 },
		"ICAO24:"
	};
	Button button_icao {
		{ 12 * 8, 2 * 16 + 12, 8 * 8, 24 },
		"012345"	// 7277A9
	};

	Text text_callsign {
		{ 4 * 8, 4 * 16 + 8, 3 * 8, 16 },
		"ID:"
	};
	Button button_callsign {
		{ 8 * 8, 4 * 16 + 4, 10 * 8, 24 },
		"" // "KOR151  "
	};
	
	Text text_frame_a {
		{ 4 * 8, 10 * 16, 14 * 8, 16 },
		"-"
	};
	Text text_frame_b {
		{ 4 * 8, 11 * 16, 14 * 8, 16 },
		"-"
	};
	
	ProgressBar progress {
		{ 5 * 8, 13 * 16, 20 * 8, 16 },
	};
	Text text_message {
		{ 5 * 8, 14 * 16, 20 * 8, 16 },
		"--------------------"
	};
	
	Button button_transmit {
		{ 2 * 8, 16 * 16, 64, 32 },
		"START"
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
