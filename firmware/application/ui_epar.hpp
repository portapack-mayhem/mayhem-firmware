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
#include "ui_receiver.hpp"
#include "clock_manager.hpp"
#include "message.hpp"
#include "rf_path.hpp"
#include "max2837.hpp"
#include "volume.hpp"
#include "transmitter_model.hpp"
#include "receiver_model.hpp"

namespace ui {

class EPARView : public View {
public:
	EPARView(NavigationView& nav);
	~EPARView();
	std::string title() const override { return "EPAR transmit"; };
	void journuit();
	
	void talk();
	void update_message();
	void focus() override;

private:
	bool txing = false;
	const rf::Frequency epar_freqs[7] = { 31325000, 31387500, 31437500, 31475000, 31687500, 31975000, 88000000 };
	char epar_bits[13];
	void on_tuning_frequency_changed(rf::Frequency f);
	
	/* |012345678901234567890123456789|
	 * |      Code ville: 000         |
	 * |          Groupe: 00          |
	 * */
	
	Text text_city {
		{ 6 * 8, 1 * 16, 11 * 8, 16 },
		"Code ville:"
	};
	NumberField city_code {
		{ 18 * 8, 1 * 16 },
		3,
		{ 0, 255 },
		0,
		' '
	};
	
	Text text_group {
		{ 10 * 8, 2 * 16, 7 * 8, 16 },
		"Groupe:"
	};
	OptionsField options_group {
		{ 18 * 8, 2 * 16 },
		4,
		{
			{ "A ", 2 },	// See receiver PCB
			{ "B ", 1 },
			{ "C ", 0 },
			{ "TP", 3 }
		}
	};
	
	Text text_freq {
		{ 5 * 8, 4 * 16, 10 * 8, 16 },
		"Frequence:"
	};
	FrequencyField field_frequency {
		{ 16 * 8, 4 * 16 },
	};
	/*OptionsField options_freq {
		{ 16 * 8, 4 * 16},
		7,
		{
			{ "31.3250", 0 },
			{ "31.3875", 1 },
			{ "31.4375", 2 },
			{ "31.4750", 3 },
			{ "31.6875", 4 },
			{ "31.9750", 5 },
			{ "TEST 88", 6 }
		}
	};*/
	
	Checkbox checkbox_ra {
		{ 7 * 8, 6 * 16 },
		8,
		"Relais 1"
	};
	Checkbox checkbox_rb {
		{ 7 * 8, 8 * 16 },
		8,
		"Relais 2"
	};
	
	NumberField excur {
		{ 12 * 8, 10 * 16 },
		4,
		{ 0, 5000 },
		20,
		' '
	};
	
	ProgressBar progressbar {
		{ 2 * 8, 12 * 16, 26 * 8, 20 },
	};
	
	Text text_debug {
		{ 5 * 8, 14 * 16, 13 * 8, 16 },
		"-------------"
	};
	
	Button button_transmit {
		{ 2 * 8, 16 * 16, 64, 32 },
		"START"
	};
	
	Checkbox checkbox_cligno {
		{ 96, 16 * 16 + 4},
		3,
		"J/N"
	};
	
	Button button_exit {
		{ 21 * 8, 16 * 16, 64, 32 },
		"Exit"
	};
};

} /* namespace ui */
