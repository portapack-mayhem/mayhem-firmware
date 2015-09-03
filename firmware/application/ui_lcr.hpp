/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

class LCRView : public View {
public:
	LCRView(NavigationView& nav, TransmitterModel& transmitter_model);
	~LCRView();
	
	void updfreq(rf::Frequency f);
	void focus() override;
	void paint(Painter& painter) override;

private:
	const char RGSB_list[37][5] = {
		"EAA0", "EAB0",	"EAC0",	"EAD0",
		"EbA0",	"EbB0",	"EbC0",	"EbD0",
		"EbE0",	"EbF0",	"EbG0",	"EbH0",
		"EbI0",	"EbJ0",	"EbK0",	"EbL0",
		"EbM0",	"EbN0",	"EbO0",	"EbP0",
		"EbS0",	"EAD0",	"AI10",	"AI20",
		"AI30",	"AI40",	"AI50",	"AI60",
		"AI70",	"AJ10",	"AJ20",	"AJ30",
		"AJ40",	"AJ50",	"AJ60",	"AJ70",
		"AK10"
	};
	char litteral[4][8];
	char rgsb[5];
	char lcrframe[256];
	rf::Frequency f = 162950000;
	TransmitterModel& transmitter_model;

	Button button_setrgsb {
		{ 16, 24, 96, 32 },
		"Set RGSB"
	};
	Button button_txsetup {
		{ 120, 24, 96, 32 },
		"TX setup"
	};

	Button button_setam_a {
		{ 16, 64, 48, 32 },
		"AM 1"
	};
	
	Button button_setam_b {
		{ 16, 64+40, 48, 32 },
		"AM 2"
	};
	
	Button button_setam_c {
		{ 16, 64+40+40, 48, 32 },
		"AM 3"
	};
	
	Button button_setam_d {
		{ 16, 64+40+40+40, 48, 32 },
		"AM 4"
	};
	
	Button button_setfreq {
		{ 8, 232, 96, 32 },
		"162.9500"
	};
	Button button_setbps {
		{ 128, 232, 96, 32 },
		"1200bps"
	};
	
	Button button_transmit {
		{ 8, 270, 48, 32 },
		"TX"
	};
	Button button_transmit_scan {
		{ 60, 270, 64, 32 },
		"SCAN TX"
	};
	Button button_exit {
		{ 176, 270, 48, 32 },
		"Exit"
	};
};

} /* namespace ui */
