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
#include "ui_textentry.hpp"
#include "ui_widget.hpp"
#include "ui_geomap.hpp"
#include "ui_tabview.hpp"
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
};

class ADSBView : public View {
public:
	ADSBView();
	
	void focus() override;
	
	void set_type(std::string type);
	void collect_frames(const uint32_t ICAO_address, std::vector<ADSBFrame>& frame_list);

protected:
	bool enabled { false };
	
	void set_enabled(bool value);
	
private:
	Checkbox check_enable {
		{ 2 * 8, 0 * 16 },
		20,
		"",
		false
	};
};

class ADSBPositionView : public ADSBView {
public:
	ADSBPositionView(NavigationView& nav);
	
	void collect_frames(const uint32_t ICAO_address, std::vector<ADSBFrame>& frame_list);

private:
	GeoPos geopos {
		{ 0, 2 * 16 }
	};
	
	Button button_set_map {
		{ 7 * 8, 7 * 16, 14 * 8, 2 * 16 },
		"Set from map"
	};
};

class ADSBCallsignView : public ADSBView {
public:
	ADSBCallsignView(NavigationView& nav);
	
	void collect_frames(const uint32_t ICAO_address, std::vector<ADSBFrame>& frame_list);

private:
	std::string callsign = "TEST1234";
	
	Labels labels_callsign {
		{ { 2 * 8, 5 * 8 }, "Callsign:", Color::light_grey() }
	};
	
	Button button_callsign {
		{ 12 * 8, 2 * 16, 10 * 8, 2 * 16 },
		""
	};
};

class ADSBSpeedView : public ADSBView {
public:
	ADSBSpeedView();
	
	void collect_frames(const uint32_t ICAO_address, std::vector<ADSBFrame>& frame_list);

private:
	Labels labels_speed {
		{ { 1 * 8, 6 * 16 }, "Speed:    kn  Bearing:    *", Color::light_grey() }
	};
	
	Compass compass {
		{ 21 * 8, 2 * 16 }
	};
	
	NumberField field_angle {
		{ 21 * 8 + 20, 6 * 16 }, 3, { 0, 359 }, 1, ' ', true
	};
	
	NumberField field_speed {
		{ 8 * 8, 6 * 16 }, 3, { 0, 999 }, 5, ' '
	};
};

class ADSBSquawkView : public ADSBView {
public:
	ADSBSquawkView();
	
	void collect_frames(const uint32_t ICAO_address, std::vector<ADSBFrame>& frame_list);

private:
	Labels labels_squawk {
		{ { 2 * 8, 2 * 16 }, "Squawk:", Color::light_grey() }
	};
	
	SymField field_squawk {
		{ 10 * 8, 2 * 16 },
		4,
		SymField::SYMFIELD_OCT
	};
};

class ADSBTxView : public View {
public:
	ADSBTxView(NavigationView& nav);
	~ADSBTxView();
	
	void focus() override;
	
	std::string title() const override { return "ADS-B transmit"; };

private:
	/*enum tx_modes {
		IDLE = 0,
		SINGLE,
		SEQUENCE
	};*/
	
	/*const float plane_lats[12] = {
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
	};*/
	
	//tx_modes tx_mode = IDLE;
	NavigationView& nav_;
	std::vector<ADSBFrame> frames { };
	
	void start_tx();
	void generate_frames();
	void rotate_frames();
	void on_txdone(const bool v);
	
	ADSBPositionView view_position { nav_ };
	ADSBCallsignView view_callsign { nav_ };
	ADSBSpeedView view_speed { };
	ADSBSquawkView view_squawk { };
	
	TabView tab_view {
		{ "Position", Color::cyan(), &view_position },
		{ "Callsign", Color::green(), &view_callsign },
		{ "Speed", Color::yellow(), &view_speed },
		{ "Squawk", Color::orange(), &view_squawk }
	};
	
	Labels labels {
		{ { 2 * 8, 4 * 8 }, "ICAO24:", Color::light_grey() }
	};
	
	SymField sym_icao {
		{ 10 * 8, 4 * 8 },
		6,
		SymField::SYMFIELD_HEX
	};
	
	Text text_frame {
		{ 1 * 8, 29 * 8, 14 * 8, 16 },
		"-"
	};
	
	TransmitterView tx_view {
		16 * 16,
		1000000,
		0
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
