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
#include "ui_tabview.hpp"
#include "ui_navigation.hpp"
#include "ui_transmitter.hpp"
#include "ui_font_fixed_8x16.hpp"

#include "bht.hpp"
#include "bitmap.hpp"
#include "message.hpp"
#include "transmitter_model.hpp"
#include "encoders.hpp"
#include "app_settings.hpp"
#include "portapack.hpp"

namespace ui {

class XylosView : public View {
public:
	XylosView(Rect parent_rect);
	
	void focus() override;
	
	void flip_relays();
	void generate_message();
	bool increment_address();
	uint32_t get_scan_remaining();
	
private:
	Labels labels {
		{ { 8 * 8, 1 * 8 }, "Header:", Color::light_grey() },
		{ { 4 * 8, 3 * 8 }, "City code:", Color::light_grey() },
		{ { 7 * 8, 5 * 8 }, "Family:", Color::light_grey() },
		{ { 2 * 8, 7 * 8 + 2 }, "Subfamily:", Color::light_grey() },
		{ { 2 * 8, 11 * 8 }, "Receiver ID:", Color::light_grey() },
		{ { 2 * 8, 14 * 8 }, "Relay:", Color::light_grey() }
	};
	
	NumberField field_header_a {
		{ 16 * 8, 1 * 8 },
		2,
		{ 0, 99 },
		1,
		'0'
	};
	NumberField field_header_b {
		{ 18 * 8, 1 * 8 },
		2,
		{ 0, 99 },
		1,
		'0'
	};
	
	NumberField field_city {
		{ 16 * 8, 3 * 8 },
		2,
		{ 0, XY_MAX_CITY },
		1,
		' '
	};
	
	NumberField field_family {
		{ 16 * 8, 5 * 8 },
		1,
		{ 0, 9 },
		1,
		' '
	};

	NumberField field_subfamily {
		{ 16 * 8, 7 * 8 + 2 },
		1,
		{ 0, 9 },
		1,
		' '
	};
	
	Checkbox checkbox_wcsubfamily {
		{ 20 * 8, 6 * 8 + 6 },
		3,
		"All"
	};
	
	NumberField field_receiver {
		{ 16 * 8, 11 * 8 },
		2,
		{ 0, 99 },
		1,
		'0'
	};
	Checkbox checkbox_wcid {
		{ 20 * 8, 10 * 8 + 4 },
		3,
		"All"
	};
	
	std::array<ImageOptionsField, 4> relay_states { };
	
	ImageOptionsField::options_t relay_options = {
		{ &bitmap_bulb_ignore, 0 },
		{ &bitmap_bulb_off, 1 },
		{ &bitmap_bulb_on, 2 }
	};
};

class EPARView : public View {
public:
	EPARView(Rect parent_rect);
	
	void focus() override;
	
	void flip_relays();
	size_t generate_message();
	bool increment_address();
	uint32_t get_scan_remaining();

	bool half { false };
	
private:
	Labels labels {
		{ { 4 * 8, 1 * 8 }, "City code:", Color::light_grey() },
		{ { 8 * 8, 3 * 8 }, "Group:", Color::light_grey() },
		{ { 8 * 8, 7 * 8 }, "Relay:", Color::light_grey() }
	};
	
	NumberField field_city {
		{ 16 * 8, 1 * 8 },
		3,
		{ 0, EPAR_MAX_CITY },
		1,
		'0'
	};
	
	OptionsField field_group {
		{ 16 * 8, 3 * 8 },
		2,
		{
			{ "A ", 2 },	// See receiver PCB
			{ "B ", 1 },
			{ "C ", 0 },
			{ "TP", 3 }
		}
	};
	
	std::array<ImageOptionsField, 2> relay_states { };
	
	ImageOptionsField::options_t relay_options = {
		{ &bitmap_bulb_off, 0 },
		{ &bitmap_bulb_on, 1 }
	};
};

class BHTView : public View {
public:
	BHTView(NavigationView& nav);
	~BHTView();

	void focus() override;
	
	std::string title() const override { return "BHT Xy/EP TX"; };

private:
	// app save settings
	std::app_settings 		settings { }; 		
	std::app_settings::AppSettings 	app_settings { };

	void on_tx_progress(const uint32_t progress, const bool done);
	void start_tx();
	void stop_tx();
	
	enum target_system_t {
		XYLOS = 0,
		EPAR = 1
	};
	
	target_system_t target_system = { };
	
	enum tx_modes {
		IDLE = 0,
		SINGLE,
		SCAN
	};
	
	tx_modes tx_mode = IDLE;
	
	Rect view_rect = { 0, 3 * 8, 240, 176 };
	
	XylosView view_xylos { view_rect };
	EPARView view_EPAR { view_rect };
	
	TabView tab_view {
		{ "Xylos", Color::cyan(), &view_xylos },
		{ "EPAR", Color::green(), &view_EPAR }
	};
	
	Labels labels {
		{ { 29 * 8, 14 * 16 + 4 }, "s", Color::light_grey() }
	};
	
	Checkbox checkbox_scan {
		{ 1 * 8, 25 * 8 },
		4,
		"Scan"
	};
	
	Checkbox checkbox_flashing {
		{ 16 * 8, 25 * 8 },
		8,
		"Flashing"
	};
	NumberField field_speed {
		{ 26 * 8, 25 * 8 + 4 },
		2,
		{ 1, 99 },
		1,
		' '
	};
	
	ProgressBar progressbar {
		{ 0 * 8, 29 * 8, 30 * 8, 16 },
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
