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

#define RDS_OFFSET_A	0b0011111100
#define RDS_OFFSET_B	0b0110011000
#define RDS_OFFSET_C	0b0101101000
#define RDS_OFFSET_Cp	0b1101010000
#define RDS_OFFSET_D	0b0110110100

namespace ui {

class RDSView : public View {
public:
	RDSView(NavigationView& nav);
	~RDSView();
	std::string title() const override { return "RDS transmit"; };

	void focus() override;
	void paint(Painter& painter) override;

private:
	char psname[9];
	
	uint8_t b2b(const bool in);
	void make_0B_group(uint32_t group[],  const uint16_t PI_code, const bool TP, const uint8_t PTY, const bool TA,
						const bool MS, const bool DI, const uint8_t C, char * TWOCHARs);

	FrequencyField field_frequency {
		{ 1 * 8, 1 * 16 },
	};
	
	OptionsField options_pty {
		{ 1 * 8, 3 * 16 },
		32,
		{
			{ "None    ", 0 },
			{ "News    ", 1 },
			{ "Affairs ", 2 },
			{ "Info    ", 3 },
			{ "Sport   ", 4 },
			{ "Educate ", 5 },
			{ "Drama   ", 6 },
			{ "Culture ", 7 },
			{ "Science ", 8 },
			{ "Varied  ", 9 },
			{ "Pop     ", 10 },
			{ "Rock    ", 11 },
			{ "Easy    ", 12 },
			{ "Light   ", 13 },
			{ "Classics", 14 },
			{ "Other   ", 15 },
			{ "Weather ", 16 },
			{ "Finance ", 17 },
			{ "Children", 18 },
			{ "Social  ", 19 },
			{ "Religion", 20 },
			{ "PhoneIn ", 21 },
			{ "Travel  ", 22 },
			{ "Leisure ", 23 },
			{ "Jazz    ", 24 },
			{ "Country ", 25 },
			{ "National", 26 },
			{ "Oldies  ", 27 },
			{ "Folk    ", 28 },
			{ "Docs    ", 29 },
			{ "AlarmTst", 30 },
			{ "Alarm   ", 31 }
		}
	};

	Button button_setpsn {
		{ 72, 92, 96, 32 },
		"Set PSN"
	};
	
	Button button_transmit {
		{ 72, 130, 96, 32 },
		"Transmit"
	};
	
	Button button_exit {
		{ 72, 270, 96, 32 },
		"Exit"
	};
};

} /* namespace ui */
