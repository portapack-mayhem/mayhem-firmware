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

#include "ui_closecall.hpp"

#include "rtc_time.hpp"
#include "event_m0.hpp"
#include "portapack.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"

#include <cstring>
#include <stdio.h>
#include <algorithm>

using namespace portapack;

namespace ui {
	
void CloseCallView::focus() {
	field_frequency_min.focus();
}

CloseCallView::~CloseCallView() {
	rtc_time::signal_tick_second -= signal_token_tick_second;
	receiver_model.disable();
	baseband::shutdown();
}

void CloseCallView::do_detection() {
	uint8_t xmax = 0;
	int16_t imax = 0;
	uint16_t iraw = 0, c;
	uint8_t power;
	rf::Frequency freq_low, freq_high;
	
	mean /= (CC_BIN_NB_NO_DC * slices_max);

	// Find max value over threshold for all slices
	for (c = 0; c < slices_max; c++) {
		power = slicemax_pow[c];
		if (power >= min_threshold) {
			if ((power - min_threshold >= mean) && (power > xmax)) {
				xmax = power;
				imax = slicemax_idx[c] + (c * CC_BIN_NB);
				iraw = slicemax_idx[c];
			}
		}
	}
	
	// Lock / release
	if ((imax >= last_channel - 2) && (imax <= last_channel + 2) && (imax)) {
		// Staying around the same frequency
		if (detect_counter >= (5 / slices_max)) {		// Todo: ugly, change. Should depend on refresh rate.
			if ((imax != locked_imax) || (!locked)) {
				std::string finalstr;
				
				if (locked) {
					// Already locked, adjust
					if (weight < 16) {
						frequency_acc += (slice_frequency + (CC_BIN_WIDTH * (imax - 120)));	// Average
						weight++;
					}
				} else {
					// Lost, locking
					frequency_acc = slice_frequency + (CC_BIN_WIDTH * (imax - 120));	// Init
					if ((frequency_acc >= f_min) && (frequency_acc <= f_max)) {

						text_infos.set("Locked !");
						big_display.set_style(&style_locked);
						
						// Approximation/error display
						freq_low = (frequency_acc - 4883) / 1000;
						freq_high = (frequency_acc + 4883) / 1000;
						finalstr = "~9.8kHz: " + to_string_dec_uint(freq_low / 1000) + "." + to_string_dec_uint(freq_low % 1000);
						finalstr += "/" + to_string_dec_uint(freq_high / 1000) + "." + to_string_dec_uint(freq_high % 1000);
						text_precision.set(finalstr);
						
						locked = true;
						weight = 1;
						locked_imax = imax;
					}
				}
				
				resolved_frequency = frequency_acc / weight;
				big_display.set(resolved_frequency);
			}
			release_counter = 0;
		} else {
			detect_counter++;
		}
	} else {
		detect_counter = 0;
		if (locked) {
			if (release_counter == 6) {
				locked = false;
				text_infos.set("Lost");
				big_display.set_style(&style_grey);
				//big_display.set(resolved_frequency);
			} else {
				release_counter++;
			}
		}
	}
	
	last_channel = imax;
	scan_counter++;
	
	portapack::display.fill_rectangle({last_pos, 90, 1, 6}, Color::black());
	if (iraw < 120)
		iraw += 2;
	else
		iraw -= 0;
	last_pos = (ui::Coord)iraw;
	portapack::display.fill_rectangle({last_pos, 90, 1, 6}, Color::red());
}

void CloseCallView::on_channel_spectrum(const ChannelSpectrum& spectrum) {
	uint8_t xmax = 0;
	int16_t imax = 0;
	uint8_t power;
	size_t i, m;
	std::array<Color, 240> pixel_row;
	
	baseband::spectrum_streaming_stop();
	
	// Draw spectrum line (for debug)
	for (i = 0; i < 120; i++) {
		const auto pixel_color = spectrum_rgb3_lut[spectrum.db[134 + i]];	// 134~253 in 0~119
		pixel_row[i] = pixel_color;
	}
	for (i = 120; i < 240; i++) {
		const auto pixel_color = spectrum_rgb3_lut[spectrum.db[i - 118]];	// 2~121 in 120~239
		pixel_row[i] = pixel_color;
	}
	display.draw_pixels(
		{ { 0, 96 + slices_counter * 4 }, { pixel_row.size(), 1 } },
		pixel_row
	);
	
	// Find max for this slice:
	
	// Check if left of slice needs to be trimmed (masked)
	//if (slices_counter == 0)
	//	i = slice_trim;
	//else
		i = 0;
	for ( ; i < 120; i++) {
		power = spectrum.db[134 + i];
		mean += power;
		if (power > xmax) {
			xmax = power;
			imax = i - 2;
		}
	}
	// Check if right of slice needs to be trimmed (masked)
	//if (slices_counter == (slices_max - 1))
	//	m = 240 - slice_trim;
	//else
		m = 240;
	for (i = 120; i < m; i++) {
		power = spectrum.db[i - 118];
		mean += power;
		if (power > xmax) {
			xmax = power;
			imax = i + 2;
		}
	}
	
	slicemax_pow[slices_counter] = xmax;
	slicemax_idx[slices_counter] = imax;
	
	// Slice update
	if (slicing) {
		if (slices_counter >= (slices_max - 1)) {
			do_detection();
			mean = 0;
			slices_counter = 0;
		} else {
			slices_counter++;
		}
		slice_frequency = slice_start + (slices_counter * CC_SLICE_WIDTH);
		receiver_model.set_tuning_frequency(slice_frequency);
	} else {
		do_detection();
	}
	
	baseband::spectrum_streaming_start();
}

void CloseCallView::on_show() {
	baseband::spectrum_streaming_start();
}

void CloseCallView::on_hide() {
	baseband::spectrum_streaming_stop();
}

void CloseCallView::on_range_changed() {
	rf::Frequency slices_span;
	rf::Frequency slice_width;
	int64_t offset;

	f_max = field_frequency_max.value();
	f_min = field_frequency_min.value();
	scan_span = abs(f_max - f_min);
	
	if (scan_span > CC_SLICE_WIDTH) {
		// ex: 100~115 (15): 102.5(97.5~107.5) -> 112.5(107.5~117.5) = 2.5 lost left and right
		slices_max = (scan_span + CC_SLICE_WIDTH - 1) / CC_SLICE_WIDTH;
		slices_span = slices_max * CC_SLICE_WIDTH;
		offset = ((scan_span - slices_span) / 2) + (CC_SLICE_WIDTH / 2);
		slice_start = std::min(f_min, f_max) + offset;
		slice_trim = 0;
		slicing = true;
		
		// Todo: trims
	} else {
		slice_frequency = (f_max + f_min) / 2;
		receiver_model.set_tuning_frequency(slice_frequency);
		
		slice_width = abs(f_max - f_min);
		slice_trim = (240 - (slice_width * 240 / CC_SLICE_WIDTH)) / 2;
		
		portapack::display.fill_rectangle({0, 97, 240, 4}, Color::black());
		portapack::display.fill_rectangle({0, 97, slice_trim, 4}, Color::orange());
		portapack::display.fill_rectangle({240 - slice_trim, 97, slice_trim, 4}, Color::orange());
		
		slices_max = 1;
		slices_counter = 0;
		slicing = false;
	}
	
	/*
	f_min = field_frequency_min.value();
	scan_span = 3000000;
	slice_frequency = (f_min + 1500000);
	slice_start = slice_frequency;
	receiver_model.set_tuning_frequency(slice_frequency);
	slice_trim = 0;
	slices_max = 1;
	slices_counter = 0;
	slicing = false;
	field_frequency_max.set_value(f_min + 3000000);
*/

	text_slices.set(to_string_dec_int(slices_max));
	slices_counter = 0;
}

void CloseCallView::on_lna_changed(int32_t v_db) {
	receiver_model.set_lna(v_db);
}

void CloseCallView::on_vga_changed(int32_t v_db) {
	receiver_model.set_vga(v_db);
}

void CloseCallView::on_tick_second() {
	// Update scan rate indication
	text_rate.set(to_string_dec_uint(scan_counter, 3));
	scan_counter = 0;
}

CloseCallView::CloseCallView(
	NavigationView& nav
)
{
	baseband::run_image(portapack::spi_flash::image_tag_wideband_spectrum);
	
	add_children({
		&text_labels_a,
		&text_labels_b,
		&text_labels_c,
		&field_frequency_min,
		&field_frequency_max,
		&field_lna,
		&field_vga,
		&field_threshold,
		&text_slices,
		&text_rate,
		&text_mhz,
		&text_infos,
		&text_precision,
		&text_debug,
		&big_display,
		&button_exit
	});
	
	text_labels_a.set_style(&style_grey);
	text_labels_b.set_style(&style_grey);
	text_labels_c.set_style(&style_grey);
	text_slices.set_style(&style_grey);
	text_rate.set_style(&style_grey);
	text_mhz.set_style(&style_grey);
	big_display.set_style(&style_grey);
	
	baseband::set_spectrum(CC_SLICE_WIDTH, 32);
	
	field_threshold.set_value(80);
	field_threshold.on_change = [this](int32_t v) {
		min_threshold = v;
	};

	field_frequency_min.set_value(receiver_model.tuning_frequency());
	field_frequency_min.set_step(100000);
	field_frequency_min.on_change = [this](rf::Frequency) {
		this->on_range_changed();
	};
	field_frequency_min.on_edit = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			//this->on_range_changed();
			this->field_frequency_min.set_value(f);
		};
	};
	
	field_frequency_max.set_value(receiver_model.tuning_frequency() + 2000000);
	field_frequency_max.set_step(100000);
	field_frequency_max.on_change = [this](rf::Frequency) {
		this->on_range_changed();
	};
	field_frequency_max.on_edit = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			//this->on_range_changed();
			this->field_frequency_max.set_value(f);
		};
	};
	
	field_lna.set_value(receiver_model.lna());
	field_lna.on_change = [this](int32_t v) {
		this->on_lna_changed(v);
	};

	field_vga.set_value(receiver_model.vga());
	field_vga.on_change = [this](int32_t v_db) {
		this->on_vga_changed(v_db);
	};
	
	on_range_changed();
	
	button_exit.on_select = [&nav](Button&){
		nav.pop();
	};
	
	signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
		this->on_tick_second();
	};

	receiver_model.set_modulation(ReceiverModel::Mode::SpectrumAnalysis);
	receiver_model.set_sampling_rate(CC_SLICE_WIDTH);
	receiver_model.set_baseband_bandwidth(2500000);
	receiver_model.enable();
}

} /* namespace ui */
