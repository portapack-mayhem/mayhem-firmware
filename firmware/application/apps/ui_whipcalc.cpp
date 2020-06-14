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

#include "ui_whipcalc.hpp"

#include "ch.h"
#include "portapack.hpp"
#include "event_m0.hpp"

#include <cstring>

using namespace portapack;

namespace ui {

void WhipCalcView::focus() {
	field_frequency.focus();
}

double ui::WhipCalcView::get_decimals(double num, int16_t mult, bool round) {
 	num -= int(num);				//keep decimals only
 	num *= mult;					//Shift decimals into integers
 	if (!round) return num;				
 	int16_t intnum = int(num);		//Round it up if necessary
 	num -= intnum;					//Get decimal part
 	if (num > .5) intnum++;			//Round up
 	return intnum;
 }

void WhipCalcView::update_result() {
	double length, calclength, divider;
	
	divider = ((double)options_type.selected_index_value() / 8.0);
	
	// Metric
	length = (speed_of_light_mps / (double)field_frequency.value()) * divider;
	
	auto m = to_string_dec_int((int)length, 2);
/* 	auto cm = to_string_dec_int(int(length * 100.0) % 100, 2);
	auto mm = to_string_dec_int(int(length * 1000.0) % 10, 1); */

 	calclength = get_decimals(length,100);				//cm
 	auto cm = to_string_dec_int(int(calclength), 2);
 	auto mm = to_string_dec_int(int(get_decimals(calclength,10,true)), 1);
	
	text_result_metric.set(m + "m " + cm + "." + mm + "cm");

	// ANT500 elements for crude adjustment
	length /= 0.14;
	if (int(length) <= 4) {
		auto elements = to_string_dec_int((int)length, 1);
		text_result_ant500.set(elements + " " + frac_str[((int(length * 10.0) % 10) + 1) / 3] + "ANT500 elements");
	} else {
		text_result_ant500.set("-");
	}

	// Imperial
	calclength = (speed_of_light_fps / (double)field_frequency.value()) * divider;
	
/* 	auto feet = to_string_dec_int((int)length, 3);
	auto inch = to_string_dec_int(int(length * 10.0) % 12, 2);
	auto inch_c = to_string_dec_int(int(length * 100.0) % 10, 1); */

	auto feet = to_string_dec_int(int(calclength), 3);
 	calclength = get_decimals(calclength,12);				//inches
 	auto inch = to_string_dec_int(int(calclength), 2);
 	auto inch_c = to_string_dec_int(int(get_decimals(calclength,10,true)), 1);
	
	text_result_imperial.set(feet + "ft " + inch + "." + inch_c + "in");
}

WhipCalcView::WhipCalcView(
	NavigationView& nav
) {

	add_children({
		&labels,
		&field_frequency,
		&options_type,
		&text_result_metric,
		&text_result_imperial,
		&text_result_ant500,
		&button_exit
	});
	
	options_type.on_change = [this](size_t, OptionsField::value_t) {
		this->update_result();
	};
	options_type.set_selected_index(2);		// Quarter wave
	
	field_frequency.set_value(transmitter_model.tuning_frequency());
	field_frequency.set_step(500000);		// 500kHz step
	field_frequency.on_change = [this](rf::Frequency) {
		this->update_result();
	};
	field_frequency.on_edit = [this, &nav]() {
		// TODO: Provide separate modal method/scheme?
		auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			this->update_result();
			this->field_frequency.set_value(f);
		};
	};
	
	button_exit.on_select = [this, &nav](Button&) {
		nav.pop();
	};
	
	update_result();
}

}
