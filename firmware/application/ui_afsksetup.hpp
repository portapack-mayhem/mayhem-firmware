/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "afsk.hpp"

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"

namespace ui {

class AFSKSetupView : public View {
public:
	AFSKSetupView(NavigationView& nav);
	
	void focus() override;
	
	std::string title() const override { return "AFSK setup"; };

private:
	void update_freq(rf::Frequency f);
	
	Text text_setfreq {
		{ 8, 32, 104, 16 },
		"Frequency:"
	};
	Button button_setfreq {
		{ 8, 48, 104, 32 },
		"----.----"
	};
	
	Text text_bps {
		{ 128, 40, 104, 16 },
		"Speed:"
	};
	OptionsField options_bps {
		{ 128, 60 },
		7,
		{
			{ "600bps ", 600 },
			{ "1200bps", 1200 },
			{ "2400bps", 2400 },
			{ "4800bps", 4800 },
			{ "9600bps", 9600 }
		}
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
		2,
		{ 1, 50 },
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
	
	Text text_format {
		{ 16, 152, 7 * 8, 16 },
		"Format:"
	};
	OptionsField options_format {
		{ 80, 152 },
		10,
		{
		}
	};
	
	Button button_save {
		{ 72, 250, 96, 40 },
		"Save"
	};
};

} /* namespace ui */
