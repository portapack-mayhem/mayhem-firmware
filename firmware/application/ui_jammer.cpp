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

#include "ui_jammer.hpp"
#include "ui_receiver.hpp"

//#include "ch.h"
//#include "evtimer.h"
#include "baseband_api.hpp"
#include "string_format.hpp"

#include "portapack_shared_memory.hpp"
#include "portapack_persistent_memory.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

namespace ui {

void JammerView::focus() {
	options_preset.focus();
}

JammerView::~JammerView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void JammerView::updfreq(uint8_t id, rf::Frequency f) {
	char finalstr[25] = {0};
	uint8_t c;
	
	auto mhz = to_string_dec_int(f / 1000000, 3);
	auto hz100 = to_string_dec_int((f / 100) % 10000, 4, '0');

	strcat(finalstr, mhz.c_str());
	strcat(finalstr, ".");
	strcat(finalstr, hz100.c_str());
	strcat(finalstr, "M");

	while (strlen(finalstr) < 10) {
		strcat(finalstr, " ");
	}

	if (id == 0) {
		range1_min = f;
		this->button_setfreq1_min.set_text(finalstr);
	}
	if (id == 1) {
		range1_max = f;
		this->button_setfreq1_max.set_text(finalstr);
	}
	if (id == 2) {
		range2_min = f;
		this->button_setfreq2_min.set_text(finalstr);
	}
	if (id == 3) {
		range2_max = f;
		this->button_setfreq2_max.set_text(finalstr);
	}
	if (id == 4) {
		range3_min = f;
		this->button_setfreq3_min.set_text(finalstr);
	}
	if (id == 5) {
		range3_max = f;
		this->button_setfreq3_max.set_text(finalstr);
	}
	
	rf::Frequency center;
	std::string bw;
	
	for (c = 0; c < 3; c++) {
		if (c == 0)	{
			center = (range1_min + range1_max) / 2;
			range1_center = center;
		}
		if (c == 1)	{
			center = (range2_min + range2_max) / 2;
			range2_center = center;
		}
		if (c == 2)	{
			center = (range3_min + range3_max) / 2;
			range3_center = center;
		}
		
		if (c == 0)	{
			range1_width = abs(range1_max - range1_min) / 1000;
			bw = to_string_dec_int(range1_width, 5);
		}
		if (c == 1)	{
			range2_width = abs(range2_max - range2_min) / 1000;
			bw = to_string_dec_int(range2_width, 5);
		}
		if (c == 2)	{
			range3_width = abs(range3_max - range3_min) / 1000;
			bw = to_string_dec_int(range3_width, 5);
		}
		
		auto center_mhz = to_string_dec_int(center / 1000000, 4);
		auto center_hz100 = to_string_dec_int((center / 100) % 10000, 4, '0');

		strcpy(finalstr, "C:");
		strcat(finalstr, center_mhz.c_str());
		strcat(finalstr, ".");
		strcat(finalstr, center_hz100.c_str());
		strcat(finalstr, "M W:");
		strcat(finalstr, bw.c_str());
		strcat(finalstr, "kHz");
		
		while (strlen(finalstr) < 23) {
			strcat(finalstr, " ");
		}
		
		if (c == 0) this->text_info1.set(finalstr);
		if (c == 1) this->text_info2.set(finalstr);
		if (c == 2) this->text_info3.set(finalstr);
	}
}

void JammerView::on_retune(const int64_t freq) {
	if (freq > 0) {
		radio::set_tuning_frequency(freq);
	}
}
	
JammerView::JammerView(NavigationView& nav) {
	baseband::run_image(portapack::spi_flash::image_tag_jammer);
	
	static constexpr Style style_val {
		.font = font::fixed_8x16,
		.background = Color::green(),
		.foreground = Color::black(),
	};
	
	static constexpr Style style_cancel {
		.font = font::fixed_8x16,
		.background = Color::red(),
		.foreground = Color::black(),
	};
	
	static constexpr Style style_info {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::grey(),
	};
	
	add_children({ {
		&text_type,
		&options_modulation,
		&text_sweep,
		&options_sweep,
		&text_preset,
		&options_preset,
		&text_hop,
		&options_hop,
		&checkbox_range1,
		&checkbox_range2,
		&checkbox_range3,
		&button_setfreq1_min,
		&button_setfreq1_max,
		&text_info1,
		&button_setfreq2_min,
		&button_setfreq2_max,
		&text_info2,
		&button_setfreq3_min,
		&button_setfreq3_max,
		&text_info3,
		&button_transmit,
		&button_exit
	} });
	
	button_transmit.set_style(&style_val);
	text_info1.set_style(&style_info);
	text_info2.set_style(&style_info);
	text_info3.set_style(&style_info);

	options_preset.set_selected_index(8);
	
	options_preset.on_change = [this](size_t n, OptionsField::value_t v) {
		(void)n;
		uint8_t c;
		
		for (c = 0; c < 3; c++) {
			updfreq(c*2, range_presets[v][c].min);
			updfreq((c*2)+1, range_presets[v][c].max);
			if (c == 0) checkbox_range1.set_value(range_presets[v][c].active);
			if (c == 1) checkbox_range2.set_value(range_presets[v][c].active);
			if (c == 2) checkbox_range3.set_value(range_presets[v][c].active);
		}
	};
	
	button_setfreq1_min.on_select = [this,&nav](Button&){
		auto new_view = nav.push<FrequencyKeypadView>(range1_min);
		new_view->on_changed = [this](rf::Frequency f) {
			updfreq(0, f);
		};
	};
	button_setfreq1_max.on_select = [this,&nav](Button&){
		auto new_view = nav.push<FrequencyKeypadView>(range1_max);
		new_view->on_changed = [this](rf::Frequency f) {
			updfreq(0, f);
		};
	};
	
	button_transmit.on_select = [this](Button&) {
		uint8_t i = 0;
		rf::Frequency t, range_lower;
		
		for (i = 0; i < 16; i++) {
			shared_memory.jammer_ranges[i].active = false;
		}
		
		// Swap
		if (range1_min > range1_max) {
			t = range1_min;
			range1_min = range1_max;
			range1_max = t;
		}
		i = 0;
		range_lower = range1_min;
//		for (i = 0; i < 3; i++) {

			if (range1_max - range_lower > 1000000) {
				shared_memory.jammer_ranges[i].center = range_lower + (1000000/2);
				shared_memory.jammer_ranges[i].width = 1000000 / 10;
				shared_memory.jammer_ranges[i].active = true;
				shared_memory.jammer_ranges[i].duration = 2280000 / 10;
				range_lower += 1000000;
			} else {
				shared_memory.jammer_ranges[i].center = (range1_max + range_lower) / 2;
				shared_memory.jammer_ranges[i].width = (range1_max - range_lower) / 10;		// ?
				shared_memory.jammer_ranges[i].active = true;
				shared_memory.jammer_ranges[i].duration = 2280000 / 10;
				//break;
			}
//		}

		// Swap
		if (range2_min > range2_max) {
			t = range2_min;
			range2_min = range2_max;
			range2_max = t;
		}
		i = 1;
		range_lower = range2_min;
//		for (i = 0; i < 3; i++) {

			if (range1_max - range_lower > 1000000) {
				shared_memory.jammer_ranges[i].center = range_lower + (1000000/2);
				shared_memory.jammer_ranges[i].width = 1000000 / 10;
				shared_memory.jammer_ranges[i].active = true;
				shared_memory.jammer_ranges[i].duration = 2280000 / 10;
				range_lower += 1000000;
			} else {
				shared_memory.jammer_ranges[i].center = (range1_max + range_lower) / 2;
				shared_memory.jammer_ranges[i].width = (range1_max - range_lower) / 10;		// ?
				shared_memory.jammer_ranges[i].active = true;
				shared_memory.jammer_ranges[i].duration = 2280000 / 10;
				//break;
			}
//		}

		// Swap
		if (range3_min > range3_max) {
			t = range3_min;
			range3_min = range3_max;
			range3_max = t;
		}
		i = 2;
		range_lower = range3_min;
//		for (i = 0; i < 3; i++) {

			if (range1_max - range_lower > 1000000) {
				shared_memory.jammer_ranges[i].center = range_lower + (1000000/2);
				shared_memory.jammer_ranges[i].width = 1000000 / 10;
				shared_memory.jammer_ranges[i].active = true;
				shared_memory.jammer_ranges[i].duration = 2280000 / 10;
				range_lower += 1000000;
			} else {
				shared_memory.jammer_ranges[i].center = (range1_max + range_lower) / 2;
				shared_memory.jammer_ranges[i].width = (range1_max - range_lower) / 10;		// ?
				shared_memory.jammer_ranges[i].active = true;
				shared_memory.jammer_ranges[i].duration = 2280000 / 10;
				//break;
			}
//		}
		
		if (jamming == true) {
			jamming = false;
			button_transmit.set_style(&style_val);
			button_transmit.set_text("START");
			radio::disable();
		} else {
			jamming = true;
			button_transmit.set_style(&style_cancel);
			button_transmit.set_text("STOP");

			/*baseband::set_jammer_data(

			);*/
			
			//transmitter_model.set_tuning_frequency(433920000);		// TODO
			transmitter_model.set_baseband_configuration({
				.mode = 0,
				.sampling_rate = 1536000U,
				.decimation_factor = 1,
			});
			transmitter_model.set_rf_amp(true);
			transmitter_model.set_baseband_bandwidth(1750000);
			transmitter_model.enable();
		}
	};

	button_exit.on_select = [&nav](Button&){
		nav.pop();
	};
}

} /* namespace ui */
