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

namespace ui {

class AFSKSetupView : public View {
public:
	AFSKSetupView(NavigationView& nav);
	
	void updfreq(rf::Frequency f);
	void focus() override;
	void paint(Painter& painter) override;

private:
	Text text_title {
		{ 40, 32, 160, 16 },
		"AFSK modulator setup"
	};
	
	Button button_setfreq {
		{ 8, 64, 104, 32 },
		"---.----M"
	};
	Button button_setbps {
		{ 128, 64, 96, 32 },
		"----bps"
	};
	
	Text text_mark {
		{ 16, 104, 48, 16 },
		"Mark:      Hz"
	};
	NumberField field_mark {
		{ 64, 104 },
		5,
		{ 100, 15000 },
		25,
		' '
	};
	
	Text text_space {
		{ 16, 120, 48, 16 },
		"Space:     Hz"
	};
	NumberField field_space {
		{ 64, 120 },
		5,
		{ 100, 15000 },
		25,
		' '
	};
	
	Text text_bw {
		{ 140, 104, 80, 16 },
		"BW:    kHz"
	};
	NumberField field_bw {
		{ 172, 104 },
		3,
		{ 1, 40 },
		1,
		' '
	};
	
	Text text_repeat {
		{ 140, 120, 64, 16 },
		"Repeat: "
	};
	NumberField field_repeat {
		{ 204, 120 },
		2,
		{ 1, 99 },
		1,
		' '
	};
	
	Checkbox checkbox_altformat {
		{ 8, 150 },
		11,
		"Alt. format"
	};
	
	Checkbox checkbox_lsb {
		{ 8, 150 },
		9,
		"LSB first"
	};
	Checkbox checkbox_parity {
		{ 8, 180 },
		11,
		"Even parity"
	};
	Checkbox checkbox_datasize {
		{ 8, 210 },
		6,
		"8 bits"
	};
	
	Button button_done {
		{ 72, 250, 96, 48 },
		"Save"
	};
};

} /* namespace ui */
