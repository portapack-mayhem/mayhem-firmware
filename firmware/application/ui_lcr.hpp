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
	char litteral[4][8];
	char rgsb[5];
	char lcrframe[256];
	uint16_t baudrate = 1200;
	rf::Frequency f = 160000000;
	TransmitterModel& transmitter_model;

	Button button_setrgsb {
		{ 16, 24, 96, 32 },
		"Set RGSB"
	};

	Button button_setam_a {
		{ 16, 64, 96, 32 },
		"AM 1"
	};
	
	Button button_setam_b {
		{ 16, 64+40, 96, 32 },
		"AM 2"
	};
	
	Button button_setam_c {
		{ 16, 64+40+40, 96, 32 },
		"AM 3"
	};
	
	Button button_setam_d {
		{ 16, 64+40+40+40, 96, 32 },
		"AM 4"
	};
	
	Button button_setfreq {
		{ 4, 232, 96, 32 },
		"160.0000"
	};
	Button button_setbaud {
		{ 4, 270, 96, 32 },
		"1200bps"
	};
	
	Button button_transmit {
		{ 120, 232, 96, 32 },
		"Transmit"
	};
	Button button_exit {
		{ 120, 270, 96, 32 },
		"Exit"
	};
};

} /* namespace ui */
