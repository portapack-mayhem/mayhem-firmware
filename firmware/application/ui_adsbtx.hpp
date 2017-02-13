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
#include "ui_transmitter.hpp"

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
	
	bool start_tx();
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
	
	Labels labels {
		{ { 2 * 8, 2 * 8 }, "Format:", Color::light_grey() },
		{ { 2 * 8, 4 * 8 }, "ICAO24:", Color::light_grey() },
		{ { 2 * 8, 7 * 8 }, "ID:", Color::light_grey() },
		{ { 2 * 8, 10 * 8 }, "Altitude:       feet", Color::light_grey() },
		{ { 2 * 8, 12 * 8 }, "Latitude:    *  '  \"", Color::light_grey() },	// No ° symbol in 8x16 font
		{ { 2 * 8, 14 * 8 }, "Longitude:   *  '  \"", Color::light_grey() },	// No ° symbol in 8x16 font
		{ { 15 * 8, 18 * 8 }, "Squawk", Color::light_grey() }
	};
	
	OptionsField options_format {
		{ 10 * 8, 1 * 16 },
		9,
		{
			{ "17: ADS-B", 17 },
			{ "18: TIS-B", 18 },
			{ "19: MIL  ", 19 },
		}
	};
	
	SymField sym_icao {
		{ 10 * 8, 2 * 16 },
		6,
		SymField::SYMFIELD_HEX
	};

	Button button_callsign {
		{ 6 * 8, 3 * 16 + 4, 10 * 8, 24 },
		""
	};
	
	NumberField field_altitude {
		{ 12 * 8, 5 * 16 },
		5,
		{ -1000, 50000 },
		250,
		' '
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
	
	NumberField field_lon_degrees {
		{ 12 * 8, 7 * 16 }, 3, { -90, 90 }, 1, ' '
	};
	NumberField field_lon_minutes {
		{ 16 * 8, 7 * 16 }, 2, { 0, 59 }, 1, ' '
	};
	NumberField field_lon_seconds {
		{ 19 * 8, 7 * 16 }, 2, { 0, 59 }, 1, ' '
	};
	
	Checkbox check_emergency {
		{ 2 * 8, 9 * 16 - 4 },
		9,
		"Emergency",
		false
	};
	
	SymField field_squawk {
		{ 22 * 8, 9 * 16 },
		4,
		SymField::SYMFIELD_OCT
	};
	
	Text text_frame_a {
		{ 2 * 8, 13 * 16, 14 * 8, 16 },
		"-"
	};
	Text text_frame_b {
		{ 2 * 8, 14 * 16, 14 * 8, 16 },
		"-"
	};
	
	TransmitterView tx_view {
		16 * 16,
		1090000000,
		2000000,
		true
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
