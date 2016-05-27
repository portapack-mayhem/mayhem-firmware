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

class LCRView : public View {
public:
	LCRView(NavigationView& nav);
	~LCRView();
	std::string title() const override { return "LCR transmit"; };
	
	void focus() override;
	void paint(Painter& painter) override;

private:
	bool txing = false;
	bool scanning = false;
	bool abort_scan = false;
	double scan_progress;
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
	int scan_index;
	
	void make_frame();
	void start_tx();
	
	const Style style_val {
		.font = font::fixed_8x16,
		.background = Color::green(),
		.foreground = Color::black(),
	};
	const Style style_cancel {
		.font = font::fixed_8x16,
		.background = Color::red(),
		.foreground = Color::black(),
	};

	Text text_recap {
		{ 8, 6, 18 * 8, 16 },
		"-"
	};
	
	NumberField adr_code {
		{ 220, 6 },
		2,
		{ 0, 36 },
		1,
		'0'
	};

	Button button_setrgsb {
		{ 16, 24, 96, 32 },
		"Set RGSB"
	};
	Button button_txsetup {
		{ 128, 24, 96, 32 },
		"TX setup"
	};
	
	Checkbox checkbox_am_a {
		{ 16, 64 },
		0,
		""
	};
	Button button_setam_a {
		{ 48, 64, 48, 24 },
		"AM 1"
	};
	
	Checkbox checkbox_am_b {
		{ 16, 96 },
		0,
		""
	};
	Button button_setam_b {
		{ 48, 96, 48, 24 },
		"AM 2"
	};
	
	Checkbox checkbox_am_c {
		{ 16, 128 },
		0,
		""
	};
	Button button_setam_c {
		{ 48, 128, 48, 24 },
		"AM 3"
	};
	
	Checkbox checkbox_am_d {
		{ 16, 160 },
		0,
		""
	};
	Button button_setam_d {
		{ 48, 160, 48, 24 },
		"AM 4"
	};
	
	Checkbox checkbox_am_e {
		{ 16, 192 },
		0,
		""
	};
	Button button_setam_e {
		{ 48, 192, 48, 24 },
		"AM 5"
	};
	
	Button button_lcrdebug {
		{ 168, 216, 56, 24 },
		"DEBUG"
	};
	
	Text text_status {
		{ 16, 224, 128, 16 },
		"Ready"
	};
	ProgressBar progress {
		{ 16, 224 + 20, 208, 16 }
	};
	
	Button button_transmit {
		{ 16, 270, 64, 32 },
		"TX"
	};
	Button button_scan {
		{ 88, 270, 64, 32 },
		"SCAN"
	};
	Button button_clear {
		{ 160, 270, 64, 32 },
		"CLEAR"
	};
};

} /* namespace ui */
