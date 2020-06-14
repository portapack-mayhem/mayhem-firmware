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

#ifndef __UI_WHIPCALC_H__
#define __UI_WHIPCALC_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_receiver.hpp"
#include "ui_navigation.hpp"
#include "string_format.hpp"

namespace ui {

class WhipCalcView : public View {
public:
	WhipCalcView(NavigationView& nav);
	
	void focus() override;
	
	std::string title() const override { return "Antenna length"; };

private:
	const double speed_of_light_mps = 299792458.0;		// m/s
	const double speed_of_light_fps = 983571087.90472;	// feet/s
	
	const std::string frac_str[4] = { "", "1/4 ", "1/2 ", "3/4 " };
	
	double get_decimals(double num, int16_t mult, bool round = false);
	void update_result();
	
	Labels labels {
		{ { 2 * 8, 2 * 16 }, "Frequency:", Color::light_grey() },
		{ { 2 * 8, 3 * 16 }, "Type:", Color::light_grey() }
	};

	FrequencyField field_frequency {
		{ 13 * 8, 2 * 16 },
	};

	OptionsField options_type {
		{ 8 * 8, 3 * 16 },
		12,
		{
			{ "Full wave", 8 },
			{ "Half wave", 4 },
			{ "Quarter wave", 2 },
			{ "3/4 wave", 6 },
			{ "1/8 wave", 1 },
			{ "3/8 wave", 3 },
			{ "5/8 wave", 5 },
			{ "7/8 wave", 7 }
		}
	};
	
	Text text_result_metric {
		{ 3 * 8, 5 * 16, 10 * 16, 16 },
		"-"
	};
	Text text_result_imperial {
		{ 2 * 8, 6 * 16, 10 * 16, 16 },
		"-"
	};
	Text text_result_ant500 {
		{ 2 * 8, 8 * 16, 26 * 16, 16 },
		"-"
	};
	
	Button button_exit {
		{ 72, 264, 96, 32 },
		"Exit"
	};
};

} /* namespace ui */

#endif/*__UI_WHIPCALC__*/
