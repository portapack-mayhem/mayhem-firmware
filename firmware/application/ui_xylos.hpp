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
#include "ui_painter.hpp"
#include "ui_menu.hpp"
#include "ui_navigation.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "clock_manager.hpp"
#include "message.hpp"
#include "rf_path.hpp"
#include "max2837.hpp"
#include "volume.hpp"
#include "transmitter_model.hpp"

namespace ui {

class XylosView : public View {
public:
	XylosView(NavigationView& nav, TransmitterModel& transmitter_model);
	~XylosView();
	void journuit();
	
	void upd_message();
	void focus() override;
	void paint(Painter& painter) override;

private:
	bool txing = false;
	const rf::Frequency xylos_freqs[7] = { 31325000, 31387500, 31437500, 31475000, 31687500, 31975000, 87000000 };
	char ccirmessage[21];

	TransmitterModel& transmitter_model;
	
	Text text_title {
		{ 1 * 8, 1 * 16, 11, 16 },
		"BH Xylos TX"
	};
	
	Text text_city {
		{ 4 * 8, 3 * 16, 11 * 8, 16 },
		"Code ville:"
	};
	NumberField city_code {
		{ 16 * 8, 3 * 16 },
		2,
		{ 0, 99 },
		1,
		' '
	};
	
	Text text_family {
		{ 7 * 8, 4 * 16, 8 * 8, 16 },
		"Famille:"
	};
	NumberField family_code {
		{ 16 * 8, 4 * 16 },
		2,
		{ 0, 9 },
		1,
		' '
	};
	
	Text text_subfamily {
		{ 2 * 8, 5 * 16 + 4, 13 * 8, 16 },
		"Sous-famille:"
	};
	NumberField subfamily_code {
		{ 16 * 8, 5 * 16 + 4 },
		2,
		{ 0, 9 },
		1,
		' '
	};
	Checkbox checkbox_wcsubfamily {
		{ 20 * 8, 5 * 16},
		"Toutes"
	};
	
	Text text_receiver {
		{ 2 * 8, 7 * 16 + 6, 13 * 8, 16 },
		"ID recepteur:"
	};
	NumberField receiver_code {
		{ 16 * 8, 7 * 16 + 6 },
		2,
		{ 0, 99 },
		1,
		' '
	};
	Checkbox checkbox_wcid {
		{ 20 * 8, 7 * 16 + 4},
		"Tous"
	};
	
	Text text_freq {
		{ 5 * 8, 9 * 16, 10 * 8, 16 },
		"Frequence:"
	};
	OptionsField options_freq {
		{ 16 * 8, 9 * 16 },
		7,
		{
			{ "31.3250", 0 },
			{ "31.3875", 1 },
			{ "31.4375", 2 },
			{ "31.4750", 3 },
			{ "31.6875", 4 },
			{ "31.9750", 5 },
			{ "TEST 87", 6 }
		}
	};
	
	Text text_ra {
		{ 12, 11 * 16, 8 * 8, 16 },
		"Relais 1"
	};
	OptionsField options_ra {
		{ 16, 12 * 16 },
		3,
		{
			{ "Ignorer", 0 },
			{ "  OFF  ", 1 },
			{ "  ON   ", 2 }
		}
	};
	Text text_rb {
		{ 88, 11 * 16, 8 * 8, 16 },
		"Relais 2"
	};
	OptionsField options_rb {
		{ 92, 12 * 16 },
		3,
		{
			{ "Ignorer", 0 },
			{ "  OFF  ", 1 },
			{ "  ON   ", 2 }
		}
	};
	Text text_rc {
		{ 164, 11 * 16, 8 * 8, 16 },
		"Relais 3"
	};
	OptionsField options_rc {
		{ 168, 12 * 16 },
		3,
		{
			{ "Ignorer", 0 },
			{ "  OFF  ", 1 },
			{ "  ON   ", 2 }
		}
	};
	
	Text text_progress {
		{ 5 * 8, 13 * 16, 20 * 8, 16 },
		"                    "
	};
	Text text_debug {
		{ 5 * 8, 14 * 16, 20 * 8, 16 },
		"--------------------"
	};
	
	Button button_transmit {
		{ 2 * 8, 16 * 16, 64, 32 },
		"START"
	};
	
	Checkbox checkbox_cligno {
		{ 96, 16 * 16 + 4},
		"J/N"
	};
	
	Button button_exit {
		{ 21 * 8, 16 * 16, 64, 32 },
		"Exit"
	};
};

} /* namespace ui */
