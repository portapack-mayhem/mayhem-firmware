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

void RangeView::focus() {
	check_enabled.focus();
}
	
extern constexpr jammer_range_t RangeView::range_presets[];
extern constexpr Style RangeView::style_info;

RangeView::RangeView(NavigationView& nav) {
	hidden(true);
	
	add_children({
		&labels,
		&check_enabled,
		&options_preset,
		&button_min,
		&button_max,
		&text_info
	});
	
	check_enabled.set_value(false);
	
	check_enabled.on_select = [this](Checkbox&, bool v) {
		frequency_range.enabled = v;
	};
	
	button_min.on_select = [this, &nav](Button& button) {
		rf::Frequency * value_ptr;

		value_ptr = &frequency_range.min;
		
		auto new_view = nav.push<FrequencyKeypadView>(*value_ptr);
		new_view->on_changed = [this, value_ptr, &button](rf::Frequency f) {
			*value_ptr = f;
			update_button(button, f);
			update_range();
		};
		
		//update_button(button, f);
	};
	
	button_max.on_select = [this, &nav](Button& button) {
		rf::Frequency * value_ptr;

		value_ptr = &frequency_range.max;
		
		auto new_view = nav.push<FrequencyKeypadView>(*value_ptr);
		new_view->on_changed = [this, value_ptr, &button](rf::Frequency f) {
			*value_ptr = f;
			update_button(button, f);
			update_range();
		};
		
		//update_button(button, f);
	};

	text_info.set_style(&style_info);
	
	options_preset.set_selected_index(8);	// ISM 868
	
	options_preset.on_change = [this](size_t, OptionsField::value_t v) {
		frequency_range.min = range_presets[v].min;
		frequency_range.max = range_presets[v].max;
		check_enabled.set_value(true);
		update_button(button_min, frequency_range.min);
		update_button(button_max, frequency_range.max);
		update_range();
	};
}

void RangeView::update_button(Button& button, rf::Frequency f) {
	std::string label;
	
	auto f_mhz = to_string_dec_int(f / 1000000, 4);
	auto f_hz100 = to_string_dec_int((f / 1000) % 1000, 3, '0');

	label = f_mhz + "." + f_hz100 + "M";
	
	button.set_text(label);
}

void RangeView::update_range() {
	std::string label;
	rf::Frequency center, bw_khz;
	
	center = (frequency_range.min + frequency_range.max) / 2;
	bw_khz = abs(frequency_range.max - frequency_range.min) / 1000;

	label = "C:" + to_string_short_freq(center) + "M  W:";
	
	if (bw_khz < 1000) {
		label += to_string_dec_int(bw_khz, 3) + "kHz";
	} else {
		bw_khz /= 1000;
		label += to_string_dec_int(bw_khz, 3) + "MHz";
	}
	
	while (label.length() < 23)
		label += " ";
	
	text_info.set(label);
}

void JammerView::focus() {
	tab_view.focus();
}

JammerView::~JammerView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void JammerView::on_retune(const rf::Frequency freq, const uint32_t range) {
	if (freq) {
		transmitter_model.set_tuning_frequency(freq);
		text_range_number.set(to_string_dec_uint(range, 2));
	}
}
	
JammerView::JammerView(
	NavigationView& nav
) : nav_ { nav }
{
	Rect view_rect = { 0, 3 * 8, 240, 80 };
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
	
	JammerChannel * jammer_channels = (JammerChannel*)shared_memory.bb_data.data;
	
	add_children({
		&tab_view,
		&view_range_a,
		&view_range_b,
		&view_range_c,
		&labels,
		&options_type,
		&text_range_number,
		&text_range_total,
		&options_speed,
		&options_hop,
		&button_transmit
	});
	
	view_range_a.set_parent_rect(view_rect);
	view_range_b.set_parent_rect(view_rect);
	view_range_c.set_parent_rect(view_rect);
	
	options_type.set_selected_index(2);		// Sweep
	options_speed.set_selected_index(3);	// 10kHz
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
			baseband::set_jammer(false, JammerType::TYPE_FSK, 0);
			jamming = false;
		} else {
			
			// Disable all ranges by default
			for (c = 0; c < JAMMER_MAX_CH; c++)
				jammer_channels[c].enabled = false;
			
			// Generate jamming "channels", maximum: JAMMER_MAX_CH
			// Convert ranges min/max to center/bw
			for (size_t r = 0; r < 3; r++) {
				
				if (range_views[r]->frequency_range.enabled) {
					range_bw = abs(range_views[r]->frequency_range.max - range_views[r]->frequency_range.min);
					
					// Sort
					if (range_views[r]->frequency_range.min < range_views[r]->frequency_range.max)
						start_freq = range_views[r]->frequency_range.min;
					else
						start_freq = range_views[r]->frequency_range.max;
					
					if (range_bw >= JAMMER_CH_WIDTH) {
						num_channels = 0;
						range_bw_sub = range_bw;
						do {
							range_bw_sub -= JAMMER_CH_WIDTH;
							num_channels++;
						} while (range_bw_sub >= JAMMER_CH_WIDTH);
						ch_width = range_bw / num_channels;
						for (c = 0; c < num_channels; c++) {
							if (i >= JAMMER_MAX_CH) {
								out_of_ranges = true;
								break;
							}
							jammer_channels[i].enabled = true;
							jammer_channels[i].width = (ch_width * 0xFFFFFFULL) / 1536000;
							jammer_channels[i].center = start_freq + (ch_width / 2) + (ch_width * c);
							jammer_channels[i].duration = 30720 * options_hop.selected_index_value();
							i++;
						}
					} else {
						if (i >= JAMMER_MAX_CH) {
							out_of_ranges = true;
						} else {
							jammer_channels[i].enabled = true;
							jammer_channels[i].width = (range_bw * 0xFFFFFFULL) / 1536000;
							jammer_channels[i].center = start_freq + (range_bw / 2);
							jammer_channels[i].duration = 30720 * options_hop.selected_index_value();
							i++;
						}
					}
				}
			}
			
			if (!out_of_ranges) {
				text_range_total.set("/" + to_string_dec_uint(i, 2));
				
				jamming = true;
				button_transmit.set_style(&style_cancel);
				button_transmit.set_text("STOP");
				
				transmitter_model.set_sampling_rate(3072000U);
				transmitter_model.set_rf_amp(true);
				transmitter_model.set_baseband_bandwidth(3500000U);
				transmitter_model.set_tx_gain(47);
				transmitter_model.enable();

				baseband::set_jammer(true, (JammerType)options_type.selected_index(), options_speed.selected_index_value());
			} else {
				nav.display_modal("Error", "Jamming bandwidth too large.\nMust be less than 24MHz.");
			}
		}
	};
}

} /* namespace ui */
