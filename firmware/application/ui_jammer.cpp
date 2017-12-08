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
#include "ui_freqman.hpp"

#include "baseband_api.hpp"
#include "string_format.hpp"

using namespace portapack;

namespace ui {

void RangeView::focus() {
	check_enabled.focus();
}

extern constexpr Style RangeView::style_info;

void RangeView::update_start(rf::Frequency f) {
	// Change everything except max
	frequency_range.min = f;
	button_start.set_text(to_string_short_freq(f));
	
	center = (frequency_range.min + frequency_range.max) / 2;
	width = abs(frequency_range.max - frequency_range.min);
	
	button_center.set_text(to_string_short_freq(center));
	button_width.set_text(to_string_short_freq(width));
}

void RangeView::update_stop(rf::Frequency f) {
	// Change everything except min
	frequency_range.max = f;
	button_stop.set_text(to_string_short_freq(f));
	
	center = (frequency_range.min + frequency_range.max) / 2;
	width = abs(frequency_range.max - frequency_range.min);
	
	button_center.set_text(to_string_short_freq(center));
	button_width.set_text(to_string_short_freq(width));
}

void RangeView::update_center(rf::Frequency f) {
	// Change min/max/center, keep width
	center = f;
	button_center.set_text(to_string_short_freq(center));

	rf::Frequency min = center - (width / 2);
	rf::Frequency max = min + width;
	
	frequency_range.min = min;
	button_start.set_text(to_string_short_freq(min));
	
	frequency_range.max = max;
	button_stop.set_text(to_string_short_freq(max));
}

void RangeView::update_width(uint32_t w) {
	// Change min/max/width, keep center
	width = w;
	
	button_width.set_text(to_string_short_freq(width));
	
	rf::Frequency min = center - (width / 2);
	rf::Frequency max = min + width;
	
	frequency_range.min = min;
	button_start.set_text(to_string_short_freq(min));
	
	frequency_range.max = max;
	button_stop.set_text(to_string_short_freq(max));
}

void RangeView::paint(Painter&) {
	// Draw lines and arrows
	Rect r;
	Point p;
	Coord c;
	
	r = button_center.screen_rect();
	p = r.center() + Point(0, r.height() / 2);
	
	display.draw_line(p, p + Point(0, 10), Color::grey());
	
	r = button_width.screen_rect();
	c = r.top() + (r.height() / 2);
	
	p = {r.left() - 64, c};
	display.draw_line({r.left(), c}, p, Color::grey());
	display.draw_line(p, p + Point(10, -10), Color::grey());
	display.draw_line(p, p + Point(10, 10), Color::grey());
	
	p = {r.right() + 64, c};
	display.draw_line({r.right(), c}, p, Color::grey());
	display.draw_line(p, p + Point(-10, -10), Color::grey());
	display.draw_line(p, p + Point(-10, 10), Color::grey());
}

RangeView::RangeView(NavigationView& nav) {
	hidden(true);
	
	add_children({
		&labels,
		&check_enabled,
		&button_load_range,
		&button_start,
		&button_stop,
		&button_center,
		&button_width
	});
	
	check_enabled.on_select = [this](Checkbox&, bool v) {
		frequency_range.enabled = v;
	};
	
	button_start.on_select = [this, &nav](Button& button) {
		auto new_view = nav.push<FrequencyKeypadView>(frequency_range.min);
		new_view->on_changed = [this, &button](rf::Frequency f) {
			update_start(f);
		};
	};
	
	button_stop.on_select = [this, &nav](Button& button) {
		auto new_view = nav.push<FrequencyKeypadView>(frequency_range.max);
		new_view->on_changed = [this, &button](rf::Frequency f) {
			update_stop(f);
		};
	};

	button_center.on_select = [this, &nav](Button& button) {
		auto new_view = nav.push<FrequencyKeypadView>(center);
		new_view->on_changed = [this, &button](rf::Frequency f) {
			update_center(f);
		};
	};
	
	button_width.on_select = [this, &nav](Button& button) {
		auto new_view = nav.push<FrequencyKeypadView>(width);
		new_view->on_changed = [this, &button](rf::Frequency f) {
			update_width(f);
		};
	};
	
	button_load_range.on_select = [this, &nav](Button&) {
		auto load_view = nav.push<FrequencyLoadView>();
		load_view->on_frequency_loaded = [this](rf::Frequency value) {
			update_center(value);
			update_width(100000);	// 100kHz default jamming bandwidth when loading unique frequency
		};
		load_view->on_range_loaded = [this](rf::Frequency start, rf::Frequency stop) {
			update_start(start);
			update_stop(stop);
		};
	};
	
	check_enabled.set_value(false);
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

void JammerView::set_jammer_channel(uint32_t i, uint32_t width, uint64_t center, uint32_t duration) {
	jammer_channels[i].enabled = true;
	jammer_channels[i].width = (width * 0xFFFFFFULL) / 1536000;
	jammer_channels[i].center = center;
	jammer_channels[i].duration = 30720 * duration;
}

extern constexpr Style JammerView::style_val;
extern constexpr Style JammerView::style_cancel;

void JammerView::start_tx() {
	uint32_t c, i = 0;
	size_t num_channels;
	rf::Frequency start_freq, range_bw, range_bw_sub, ch_width;
	bool out_of_ranges = false;
	
	size_t hop_value = options_hop.selected_index_value();
	
	// Disable all channels by default
	for (c = 0; c < JAMMER_MAX_CH; c++)
		jammer_channels[c].enabled = false;
	
	// Generate jamming channels with JAMMER_MAX_CH maximum width
	// Convert ranges min/max to center/bw
	for (size_t r = 0; r < 3; r++) {
		
		if (range_views[r]->frequency_range.enabled) {
			range_bw = abs(range_views[r]->frequency_range.max - range_views[r]->frequency_range.min);
			
			// Get lower bound
			if (range_views[r]->frequency_range.min < range_views[r]->frequency_range.max)
				start_freq = range_views[r]->frequency_range.min;
			else
				start_freq = range_views[r]->frequency_range.max;
			
			if (range_bw >= JAMMER_CH_WIDTH) {
				// Split range in multiple channels
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
					set_jammer_channel(i, ch_width, start_freq + (ch_width / 2) + (ch_width * c), hop_value);
					i++;
				}
			} else {
				// Range fits in a single channel
				if (i >= JAMMER_MAX_CH) {
					out_of_ranges = true;
				} else {
					set_jammer_channel(i, range_bw, start_freq + (range_bw / 2), hop_value);
					i++;
				}
			}
		}
	}
	
	if (!out_of_ranges && i) {
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
		if (out_of_ranges)
			nav_.display_modal("Error", "Jamming bandwidth too large.\nMust be less than 24MHz.");
		else
			nav_.display_modal("Error", "No range enabled.");
	}
}

void JammerView::stop_tx() {
	button_transmit.set_style(&style_val);
	button_transmit.set_text("START");
	transmitter_model.disable();
	radio::disable();
	baseband::set_jammer(false, JammerType::TYPE_FSK, 0);
	jamming = false;
}
	
JammerView::JammerView(
	NavigationView& nav
) : nav_ { nav }
{
	Rect view_rect = { 0, 3 * 8, 240, 80 };
	baseband::run_image(portapack::spi_flash::image_tag_jammer);
	
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
	
	options_type.set_selected_index(3);		// Rand CW
	options_speed.set_selected_index(3);	// 10kHz
	options_hop.set_selected_index(1);		// 50ms
	button_transmit.set_style(&style_val);

	button_transmit.on_select = [this](Button&) {
		if (jamming)
			stop_tx();
		else
			start_tx();
	};
}

} /* namespace ui */
