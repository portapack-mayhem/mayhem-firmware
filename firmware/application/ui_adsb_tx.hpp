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
#include "adsb.hpp"
#include "utility.hpp"
#include "sine_table.hpp"
#include "ui_textentry.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_transmitter.hpp"

#include "message.hpp"
#include "transmitter_model.hpp"
#include "portapack.hpp"

using namespace adsb;

namespace ui {

class Compass : public Widget {
public:
	Compass(const Point parent_pos);

	void set_value(uint32_t new_value);

	void paint(Painter&) override;

private:
	const range_t<uint32_t> range { 0, 359 };
	uint32_t value_ { 0 };

	uint32_t clamp_value(uint32_t value);
};

class ADSBTxView : public View {
public:
	ADSBTxView(NavigationView& nav);
	~ADSBTxView();
	
	void paint(Painter&) override;
	
	void focus() override;
	
	std::string title() const override { return "ADS-B transmit"; };

private:
	enum tx_modes {
		IDLE = 0,
		SINGLE,
		SEQUENCE
	};
	
	const float plane_lats[12] = {
		0,
		-1,
		-2,
		-3,
		-4,
		-5,
		-4.5,
		-5,
		-4,
		-3,
		-2,
		-1
	};
	const float plane_lons[12] = {
		0,
		1,
		1,
		1,
		2,
		1,
		0,
		-1,
		-2,
		-1,
		-1,
		-1
	};
	
	tx_modes tx_mode = IDLE;
	
	std::string callsign = "TEST1234";
	
	ADSBFrame frames[4] { };
	
	void start_tx();
	void generate_frames();
	//void rotate_frames();
	void on_txdone(const bool v);
	
	Labels labels {
		{ { 2 * 8, 2 * 8 }, "Format: 17 (ADS-B)", Color::light_grey() },
		{ { 2 * 8, 4 * 8 }, "ICAO24:", Color::light_grey() },
		{ { 2 * 8, 7 * 8 }, "Callsign:", Color::light_grey() },
		{ { 1 * 8, 11 * 8 }, "Alt:       feet", Color::light_grey() },
		{ { 1 * 8, 13 * 8 }, "Lat:     *  '  \"", Color::light_grey() },	// No ° symbol in 8x16 font
		{ { 1 * 8, 15 * 8 }, "Lon:     *  '  \"", Color::light_grey() },
		{ { 1 * 8, 18 * 8 }, "Speed:    kn  Bearing:    *", Color::light_grey() },
		{ { 16 * 8, 22 * 8 }, "Squawk:", Color::light_grey() }
	};
	
	// Only ADS-B is implemented right now
	/*OptionsField options_format {
		{ 10 * 8, 1 * 16 },
		9,
		{
			{ "17: ADS-B", 17 },
			{ "18: TIS-B", 18 },
			{ "19: MIL  ", 19 },
		}
	};*/
	
	SymField sym_icao {
		{ 10 * 8, 2 * 16 },
		6,
		SymField::SYMFIELD_HEX
	};

	Button button_callsign {
		{ 12 * 8, 3 * 16 + 4, 10 * 8, 24 },
		""
	};
	
	NumberField field_altitude {
		{ 6 * 8, 11 * 8 },
		5,
		{ -1000, 50000 },
		250,
		' '
	};
	
	NumberField field_lat_degrees {
		{ 6 * 8, 13 * 8 }, 4, { -90, 90 }, 1, ' '
	};
	NumberField field_lat_minutes {
		{ 11 * 8, 13 * 8 }, 2, { 0, 59 }, 1, ' '
	};
	NumberField field_lat_seconds {
		{ 14 * 8, 13 * 8 }, 2, { 0, 59 }, 1, ' '
	};
	
	NumberField field_lon_degrees {
		{ 6 * 8, 15 * 8 }, 4, { -180, 180 }, 1, ' '
	};
	NumberField field_lon_minutes {
		{ 11 * 8, 15 * 8 }, 2, { 0, 59 }, 1, ' '
	};
	NumberField field_lon_seconds {
		{ 14 * 8, 15 * 8 }, 2, { 0, 59 }, 1, ' '
	};
	
	Compass compass {
		{ 21 * 8, 5 * 16 }
	};
	NumberField field_angle {
		{ 21 * 8 + 20, 9 * 16 }, 3, { 0, 359 }, 1, ' '
	};
	
	NumberField field_speed {
		{ 8 * 8, 18 * 8 }, 3, { 0, 999 }, 5, ' '
	};
	
	Checkbox check_emergency {
		{ 2 * 8, 11 * 16 - 4 },
		9,
		"Emergency",
		false
	};
	SymField field_squawk {
		{ 24 * 8, 11 * 16 },
		4,
		SymField::SYMFIELD_OCT
	};
	
	Text text_frame {
		{ 1 * 8, 29 * 8, 14 * 8, 16 },
		"-"
	};
	
	TransmitterView tx_view {
		16 * 16,
		0,
		0,
		true
	};
	
	MessageHandlerRegistration message_handler_tx_done {
		Message::ID::TXDone,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const TXDoneMessage*>(p);
			this->on_txdone(message.done);
		}
	};
};

} /* namespace ui */
