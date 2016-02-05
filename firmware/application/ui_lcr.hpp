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
	LCRView(NavigationView& nav);
	~LCRView();
	
	void make_frame();
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
	char litteral[5][8];
	char rgsb[5];
	char lcrstring[256];
	char checksum = 0;
	char lcrframe[256];
	char lcrframe_f[256];
	rf::Frequency f;

	Text text_status {
		{ 168, 196, 64, 16 },
		"Ready"
	};

	Text text_recap {
		{ 32, 6, 192, 16 },
		"-"
	};

	Button button_setrgsb {
		{ 16, 24, 96, 32 },
		"Set RGSB"
	};
	Button button_txsetup {
		{ 120, 24, 96, 32 },
		"TX setup"
	};
	
	Checkbox checkbox_am_a {
		{ 16, 68 },
		20,
		""
	};
	Button button_setam_a {
		{ 48, 64, 48, 32 },
		"AM 1"
	};
	Checkbox checkbox_am_b {
		{ 16, 68+40 },
		20,
		""
	};
	Button button_setam_b {
		{ 48, 64+40, 48, 32 },
		"AM 2"
	};
	Checkbox checkbox_am_c {
		{ 16, 68+40+40 },
		20,
		" "
	};
	Button button_setam_c {
		{ 48, 64+40+40, 48, 32 },
		"AM 3"
	};
	Checkbox checkbox_am_d {
		{ 16, 68+40+40+40 },
		20,
		""
	};
	Button button_setam_d {
		{ 48, 64+40+40+40, 48, 32 },
		"AM 4"
	};
	Checkbox checkbox_am_e {
		{ 16, 68+40+40+40+40 },
		20,
		""
	};
	Button button_setam_e {
		{ 48, 64+40+40+40+40, 48, 32 },
		"AM 5"
	};
	
	Button button_lcrdebug {
		{ 166, 224, 56, 32 },
		"DEBUG"
	};
	
	Button button_transmit {
		{ 24, 270, 48, 32 },
		"TX"
	};
	Button button_transmit_scan {
		{ 76, 270, 72, 32 },
		"SCAN TX"
	};
	Button button_exit {
		{ 176, 270, 48, 32 },
		"Exit"
	};
};

} /* namespace ui */
