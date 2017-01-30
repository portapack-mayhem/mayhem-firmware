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

#define JAMMER_CH_WIDTH 500000

using namespace portapack;

namespace ui {

void JammerView::focus() {
	options_preset.focus();
}

JammerView::~JammerView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void JammerView::update_range(const uint32_t n) {
	std::string label;
	jammer_range_t * range_ptr;
	rf::Frequency center, bw_khz;
	
	range_ptr = &frequency_range[n];
	
	center = (range_ptr->min + range_ptr->max) / 2;
	bw_khz = abs(range_ptr->max - range_ptr->min) / 1000;
	
	auto center_mhz = to_string_dec_int(center / 1000000, 4);
	auto center_hz100 = to_string_dec_int((center / 1000) % 1000, 3, '0');

	label = "C:" + center_mhz + "." + center_hz100 + "M   W:";
	
	if (bw_khz < 1000) {
		label += to_string_dec_int(bw_khz, 3) + "kHz";
	} else {
		bw_khz /= 1000;
		label += to_string_dec_int(bw_khz, 3) + "MHz";
	}
	
	while (label.length() < 23)
		label += " ";
	
	texts_info[n].set(label);
}

void JammerView::update_button(const uint32_t n) {
	std::string label;
	rf::Frequency f;
	
	if (n & 1)
		f = frequency_range[n / 2].max;
	else
		f = frequency_range[n / 2].min;
	
	auto f_mhz = to_string_dec_int(f / 1000000, 4);
	auto f_hz100 = to_string_dec_int((f / 1000) % 1000, 3, '0');

	label = f_mhz + "." + f_hz100 + "M";
	
	buttons_freq[n].set_text(label);
}

void JammerView::on_retune(const rf::Frequency freq, const uint32_t range) {
	if (freq) {
		transmitter_model.set_tuning_frequency(freq);
		text_range_number.set(to_string_dec_uint(range, 1));
	}
}
	
JammerView::JammerView(NavigationView& nav) {
	size_t n;
	
	baseband::run_image(portapack::spi_flash::image_tag_jammer);
	
	static constexpr Style style_val {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::green(),
	};
	
	static constexpr Style style_cancel {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::red(),
	};
	
	static constexpr Style style_info {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::grey(),
	};
	
	JammerChannel * jammer_channels = (JammerChannel*)shared_memory.bb_data.data;
	
	add_children({
		&text_type,
		&options_type,
		&text_range_number,
		&text_range_total,
		&text_speed,
		&options_speed,
		&text_preset,
		&options_preset,
		&text_hop,
		&options_hop,
		&button_transmit,
		&button_exit
	});
	
	const auto button_freq_fn = [this, &nav](Button& button) {
		rf::Frequency * value_ptr;
		
		if (button.id & 1)
			value_ptr = &frequency_range[id].max;
		else
			value_ptr = &frequency_range[id].min;
		
		auto new_view = nav.push<FrequencyKeypadView>(*value_ptr);
		new_view->on_changed = [this, value_ptr](rf::Frequency f) {
			*value_ptr = f;
		};
	};
	
	const auto checkbox_fn = [this](Checkbox& checkbox, bool v) {
		frequency_range[checkbox.id].enabled = v;
	};
	
	n = 0;
	for (auto& button : buttons_freq) {
		button.on_select = button_freq_fn;
		button.set_parent_rect({
			static_cast<Coord>(13 * 8),
			static_cast<Coord>(76 + ((n >> 1) * 58) + (20 * (n & 1))),
			96, 20
		});
		button.id = n;
		add_child(&button);
		n++;
	}
	
	n = 0;
	for (auto& checkbox : checkboxes) {
		checkbox.on_select = checkbox_fn;
		checkbox.set_parent_rect({
			static_cast<Coord>(8),
			static_cast<Coord>(86 + (n * 58)),
			24, 24
		});
		checkbox.id = n;
		checkbox.set_text("Range " + to_string_dec_uint(n + 1));
		add_child(&checkbox);
		n++;
	}
	
	n = 0;
	for (auto& text : texts_info) {
		text.set_parent_rect({
			static_cast<Coord>(3 * 8),
			static_cast<Coord>(116 + (n * 58)),
			25 * 8, 16
		});
		text.set_style(&style_info);
		add_child(&text);
		n++;
	}
	
	options_preset.on_change = [this](size_t, OptionsField::value_t v) {
		const jammer_range_t * preset_ptr;
		uint32_t c;
		
		for (c = 0; c < 3; c++) {
			preset_ptr = &range_presets[v][c];
			
			frequency_range[c].min = preset_ptr->min;
			frequency_range[c].max = preset_ptr->max;
			checkboxes[c].set_value(preset_ptr->enabled);
			update_button(c * 2);
			update_button(c * 2 + 1);
			update_range(c);
		}
		
	};
	
	options_type.set_selected_index(1);		// Noise
	options_speed.set_selected_index(2);	// 10kHz
	options_preset.set_selected_index(8);	// ISM 868
	options_hop.set_selected_index(1);		// 50ms
	button_transmit.set_style(&style_val);

	button_transmit.on_select = [this, &nav, jammer_channels](Button&) {
		uint32_t c, i = 0;
		size_t num_channels;
		rf::Frequency start_freq, range_bw, range_bw_sub, ch_width;
		bool out_of_ranges = false;
		
		if (jamming) {
			button_transmit.set_style(&style_val);
			button_transmit.set_text("START");
			transmitter_model.disable();
			radio::disable();
			baseband::set_jammer(false, 0, 0);
			jamming = false;
		} else {
			
			// Disable all ranges by default
			for (c = 0; c < 9; c++)
				jammer_channels[c].enabled = false;
			
			// Generate jamming "channels", maximum: 9
			// Convert ranges min/max to center/bw
			for (size_t r = 0; r < 3; r++) {
				
				if (frequency_range[r].enabled) {
					range_bw = abs(frequency_range[r].max - frequency_range[r].min);
					
					// Sort
					if (frequency_range[r].min < frequency_range[r].max)
						start_freq = frequency_range[r].min;
					else
						start_freq = frequency_range[r].max;
					
					if (range_bw >= JAMMER_CH_WIDTH) {
						num_channels = 0;
						range_bw_sub = range_bw;
						do {
							range_bw_sub -= JAMMER_CH_WIDTH;
							num_channels++;
						} while (range_bw_sub >= JAMMER_CH_WIDTH);
						ch_width = range_bw / num_channels;
						for (c = 0; c < num_channels; c++) {
							if (i >= 9) {
								out_of_ranges = true;
								break;
							}
							jammer_channels[i].enabled = true;
							jammer_channels[i].width = ch_width;
							jammer_channels[i].center = start_freq + (ch_width / 2) + (ch_width * c);
							jammer_channels[i].duration = 15360 * options_hop.selected_index_value();
							i++;
						}
					} else {
						if (i >= 9) {
							out_of_ranges = true;
						} else {
							jammer_channels[i].enabled = true;
							jammer_channels[i].width = range_bw;
							jammer_channels[i].center = start_freq + (range_bw / 2);
							jammer_channels[i].duration = 15360 * options_hop.selected_index_value();
							i++;
						}
					}
				}
			}
			
			if (!out_of_ranges) {
				text_range_total.set("/" + to_string_dec_uint(i, 1));
				
				jamming = true;
				button_transmit.set_style(&style_cancel);
				button_transmit.set_text("STOP");
				
				transmitter_model.set_sampling_rate(1536000U);
				transmitter_model.set_rf_amp(true);
				transmitter_model.set_baseband_bandwidth(1750000);
				transmitter_model.set_tx_gain(47);
				transmitter_model.enable();

				baseband::set_jammer(true, options_type.selected_index(), options_speed.selected_index());
			} else {
				nav.display_modal("Error", "Jamming bandwidth too large.");
			}
		}
	};

	button_exit.on_select = [&nav](Button&){
		nav.pop();
	};
}

} /* namespace ui */
