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

void JammerView::update_text(uint8_t id, rf::Frequency f) {
	char finalstr[25] = {0};
	rf::Frequency center;
	std::string bw;
	uint8_t c;

	/*
	auto mhz = to_string_dec_int(f / 1000000, 3);
	auto hz100 = to_string_dec_int((f / 100) % 10000, 4, '0');

	strcat(finalstr, mhz.c_str());
	strcat(finalstr, ".");
	strcat(finalstr, hz100.c_str());
	strcat(finalstr, "M");

	while (strlen(finalstr) < 10)
		strcat(finalstr, " ");

	buttons_freq[id].set_text(finalstr);
	*/
	
	for (c = 0; c < 3; c++) {
		center = (frequency_range[c].min + frequency_range[c].max) / 2;
		bw = to_string_dec_int(abs(frequency_range[c].max - frequency_range[c].min) / 1000, 5);
		
		auto center_mhz = to_string_dec_int(center / 1000000, 4);
		auto center_hz100 = to_string_dec_int((center / 100) % 10000, 4, '0');

		strcpy(finalstr, "C:");
		strcat(finalstr, center_mhz.c_str());
		strcat(finalstr, ".");
		strcat(finalstr, center_hz100.c_str());
		strcat(finalstr, "M W:");
		strcat(finalstr, bw.c_str());
		strcat(finalstr, "kHz");
		
		while (strlen(finalstr) < 23)
			strcat(finalstr, " ");
		
		if (c == 0) text_info1.set(finalstr);
		if (c == 1) text_info2.set(finalstr);
		if (c == 2) text_info3.set(finalstr);
	}
}

void JammerView::on_retune(const int64_t freq) {
	if (freq > 0)
		transmitter_model.set_tuning_frequency(freq);
}
	
JammerView::JammerView(NavigationView& nav) {
	size_t n;
	
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
	
	JammerRange * jammer_ranges = (JammerRange*)shared_memory.tx_data;
	
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
		&text_info1,
		&text_info2,
		&text_info3,
		&button_transmit,
		&button_exit
	} });
	
	const auto button_freq_fn = [this, &nav](Button& button) {
		uint16_t id = button.id;
		rf::Frequency * value_ptr;
		
		if (id & 1)
			value_ptr = &frequency_range[id].max;
		else
			value_ptr = &frequency_range[id].min;
		auto new_view = nav.push<FrequencyKeypadView>(*value_ptr);
		new_view->on_changed = [this, value_ptr](rf::Frequency f) {
			*value_ptr = f;
		};
	};
	
	n = 0;
	for (auto& button : buttons_freq) {
		button.on_select = button_freq_fn;
		button.set_parent_rect({
			static_cast<Coord>(13 * 8),
			static_cast<Coord>(((n >> 1) * 52) + 90 + (18 * (n & 1))),
			88, 18
		});
		button.id = n;
		button.set_text("----.----M");
		add_child(&button);
		n++;
	}
	
	button_transmit.set_style(&style_val);
	text_info1.set_style(&style_info);
	text_info2.set_style(&style_info);
	text_info3.set_style(&style_info);
	
	options_preset.on_change = [this](size_t n, OptionsField::value_t v) {
		(void)n;
		
		for (uint8_t c = 0; c < 3; c++) {
			frequency_range[c].min = range_presets[v][c].min;
			frequency_range[c].max = range_presets[v][c].max;
		}
		checkbox_range1.set_value(range_presets[v][0].enabled);
		checkbox_range2.set_value(range_presets[v][1].enabled);
		checkbox_range3.set_value(range_presets[v][2].enabled);
		
		update_text(0, 0);
	};
	
	options_preset.set_selected_index(8);		// Sigfox, because they deserve it

	button_transmit.on_select = [this, &nav, jammer_ranges](Button&) {
		uint8_t c, i = 0;
		size_t num_ranges;
		rf::Frequency start_freq, range_bw, ch_width;
		bool out_of_ranges = false;
		
		// Disable all ranges by default
		for (i = 0; i < 9; i++)
			jammer_ranges[i].enabled = false;
		
		// Generate jamming "channels", maximum: 9
		// Convert ranges min/max to center/bw
		for (c = 0; c < 3; c++) {
			range_bw = abs(frequency_range[c].max - frequency_range[c].min);		// Total bw for range
			if (frequency_range[c].min < frequency_range[c].max)
				start_freq = frequency_range[c].min;
			else
				start_freq = frequency_range[c].max;
			if (range_bw > 500000) {
				// Example: 600kHz
				// int(600000 / 500000) = 2
				// CH-BW = 600000 / 2 = 300000
				// Center-A = min + CH-BW / 2 = 150000
				// BW-A = CH-BW = 300000
				// Center-B = min + CH-BW + Center-A = 450000
				// BW-B = CH-BW = 300000
				num_ranges = 0;
				while (range_bw > 500000) {
					range_bw -= 500000;
					num_ranges++;
				}
				ch_width = range_bw / num_ranges;
				for (c = 0; c < num_ranges; c++) {
					if (i >= 9) {
						out_of_ranges = true;
						break;
					}
					jammer_ranges[i].enabled = true;
					jammer_ranges[i].width = ch_width;
					jammer_ranges[i].center = start_freq + (ch_width / 2) + (ch_width * c);
					jammer_ranges[i].duration = 2280000 / 10;				// ?
					i++;
				}
			} else {
				if (i >= 9) {
					out_of_ranges = true;
				} else {
					jammer_ranges[i].enabled = true;
					jammer_ranges[i].width = range_bw;
					jammer_ranges[i].center = start_freq + (range_bw / 2);
					jammer_ranges[i].duration = 2280000 / 10;					// ?
					i++;
				}
			}
			
			if (!out_of_ranges) {
				if (jamming == true) {
					jamming = false;
					button_transmit.set_style(&style_val);
					button_transmit.set_text("START");
					radio::disable();
				} else {
					jamming = true;
					button_transmit.set_style(&style_cancel);
					button_transmit.set_text("STOP");
					
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
			} else {
				nav.display_modal("Error", "Jamming bandwidth too high.");
			}
		}
	};

	button_exit.on_select = [&nav](Button&){
		nav.pop();
	};
}

} /* namespace ui */
