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
	
	void start_tx();
	void generate_frame();
	void generate_frame_pos();
	void on_txdone(const int n);
	
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
	
	Text text_format {
		{ 2 * 8, 1 * 16, 7 * 8, 16 },
		"Format:"
	};
	OptionsField options_format {
		{ 10 * 8, 1 * 16 },
		10,
		{
			{ "17: ADS-B", 17 },
			{ "18: TIS-B", 18 },
			{ "19: Don't.", 19 },
		}
	};
	
	Text text_icaolabel {
		{ 2 * 8, 2 * 16, 7 * 8, 16 },
		"ICAO24:"
	};
	SymField sym_icao {
		{ 10 * 8, 2 * 16 },
		6,
		true	// Hex
	};

	Text text_callsign {
		{ 2 * 8, 3 * 16 + 8, 3 * 8, 16 },
		"ID:"
	};
	Button button_callsign {
		{ 6 * 8, 3 * 16 + 4, 10 * 8, 24 },
		""
	};
	
	Text text_altitude {
		{ 2 * 8, 5 * 16, 20 * 8, 16 },
		"Altitude:       feet"
	};
	NumberField field_altitude {
		{ 12 * 8, 5 * 16 },
		5,
		{ -1000, 50000 },
		250,
		' '
	};
	
	Text text_latitude {
		{ 2 * 8, 6 * 16, 20 * 8, 16 },
		"Latitude:    *  '  \""		// No ° symbol in 8x16 font
	};
	NumberField field_lat_degrees {
		{ 12 * 8, 6 * 16 }, 3, { -90, 90 }, 1, ' '
	};
	NumberField field_lat_minutes {
		{ 16 * 8, 6 * 16 }, 2, { 0, 59 }, 1, ' '
	};
	NumberField field_lat_seconds {
		{ 19 * 8, 6 * 16 }, 2, { 0, 59 }, 1, ' '
	};
	Text text_longitude {
		{ 2 * 8, 7 * 16, 20 * 8, 16 },
		"Longitude:   *  '  \""		// No ° symbol in 8x16 font
	};
	NumberField field_lon_degrees {
		{ 12 * 8, 7 * 16 }, 3, { -90, 90 }, 1, ' '
	};
	NumberField field_lon_minutes {
		{ 16 * 8, 7 * 16 }, 2, { 0, 59 }, 1, ' '
	};
	NumberField field_lon_seconds {
		{ 19 * 8, 7 * 16 }, 2, { 0, 59 }, 1, ' '
	};
	
	Text text_frame_a {
		{ 4 * 8, 12 * 16, 14 * 8, 16 },
		"-"
	};
	Text text_frame_b {
		{ 4 * 8, 13 * 16, 14 * 8, 16 },
		"-"
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
