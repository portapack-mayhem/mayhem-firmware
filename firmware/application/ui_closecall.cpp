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
#include "msgpack.hpp"

#include "ch.h"
#include "time.hpp"

#include "event_m0.hpp"
#include "hackrf_gpio.hpp"
#include "portapack.hpp"
#include "radio.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"

#include <cstring>
#include <stdio.h>
#include <algorithm>

using namespace portapack;

namespace ui {
	
void CloseCallView::focus() {
	button_exit.focus();
}

CloseCallView::~CloseCallView() {
	receiver_model.disable();
	time::signal_tick_second -= signal_token_tick_second;
}

void CloseCallView::do_detection() {
	uint8_t xmax = 0;
	int64_t imax = 0;
	uint16_t iraw = 0, c;
	uint8_t power;
	rf::Frequency freq_low, freq_high;
	
	mean /= (CC_BIN_NB * slices_max);

	// Find max value over threshold from all slices
	for (c = 0; c < slices_max; c++) {
		power = slicemax_db[c];
		if (power >= min_threshold) {
			if ((power - min_threshold >= mean) && (power > xmax)) {
				xmax = power;
				imax = slicemax_idx[c] + (c * CC_BIN_NB);
				iraw = slicemax_idx[c];
			}
		}
	}
	
	// e-Message POCSAG FR:
	// 466.025 MHz - 466.05 MHz - 466.075 MHz - 466.175 MHz - 466.20625 MHz - 466.23125 MHz
	// 25 25 100 31 25
	
	// Catches:
	// 466.000
	// 466.021  21
	// 466.042	21
	// 466.148	106
	// 466.169	21
	// 466.190	21
	
	// Lock / release
	if ((imax >= last_channel - 2) && (imax <= last_channel + 2) && imax) {
		// Staying around the same frequency (+/- 25.4kHz)
		if (detect_counter >= (5 / slices_max)) {
			if ((imax != locked_imax) || (!locked)) {
				char finalstr[29] = {0};
				
				// 236 steps = 3MHz
				// Resolution = 12.7kHz
				if (locked)
					resolved_frequency = (resolved_frequency + slice_start + (CC_BIN_WIDTH * (imax - 118))) / 2;	// Mean
				else
					resolved_frequency = slice_start + (CC_BIN_WIDTH * (imax - 118));	// Init
				
				//text_debug.set(to_string_dec_int(CC_BIN_WIDTH * (imax - 118)));
				
				// Correct according to DC spike mask width (4 for now)
				if (iraw > 118)
					resolved_frequency -= (2 * CC_BIN_WIDTH);
				else
					resolved_frequency += (2 * CC_BIN_WIDTH);

				text_infos.set("Locked !");
				big_display.set_style(&style_locked);
				big_display.set(resolved_frequency);
				
				// Approximation/error display
				freq_low = (resolved_frequency - 6355) / 1000;
				freq_high = (resolved_frequency + 6355) / 1000;
				strcat(finalstr, "~12.7kHz ");
				strcat(finalstr, to_string_dec_uint(freq_low / 1000).c_str());
				strcat(finalstr, ".");
				strcat(finalstr, to_string_dec_uint(freq_low % 1000).c_str());
				strcat(finalstr, "/");
				strcat(finalstr, to_string_dec_uint(freq_high / 1000).c_str());
				strcat(finalstr, ".");
				strcat(finalstr, to_string_dec_uint(freq_high % 1000).c_str());
				text_precision.set(finalstr);
				
				locked = true;
				locked_imax = imax;
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
				text_infos.set("Lost    ");
				big_display.set_style(&style_grey);
				big_display.set(resolved_frequency);
			} else {
				release_counter++;
			}
		}
	}
	
	last_channel = imax;
	scan_counter++;
	
	portapack::display.fill_rectangle({last_pos, 90, 1, 13}, Color::black());
	last_pos = (ui::Coord)(iraw);
	portapack::display.fill_rectangle({last_pos, 90, 1, 13}, Color::red());
}

void CloseCallView::on_channel_spectrum(const ChannelSpectrum& spectrum) {
	uint8_t xmax = 0;
	uint16_t imax = 0;
	uint8_t threshold;
	size_t i, m;
	
	baseband::spectrum_streaming_stop();
	
	// Spectrum line (for debug)
	std::array<Color, 240> pixel_row;
	for(i = 0; i < 118; i++) {
		const auto pixel_color = spectrum_rgb3_lut[spectrum.db[256 - 120 + i]];
		pixel_row[i + 2] = pixel_color;
	}

	for(i = 122; i < 240; i++) {
		const auto pixel_color = spectrum_rgb3_lut[spectrum.db[i - 120]];
		pixel_row[i - 2] = pixel_color;
	}

	display.draw_pixels(
		{ { 0, 96 + slices_counter * 4 }, { pixel_row.size(), 1 } },
		pixel_row
	);
	
	// Find max for this slice:
	
	// Check if left of slice needs to be trimmed (masked)
	if (slices_counter == 0)
		i = slice_trim;
	else
		i = 0;
	for ( ; i < 118; i++) {
		threshold = spectrum.db[256 - 120 + i];		// 128+8 = 136 ~254
		if (threshold > xmax) {
			xmax = threshold;
			imax = i;
		}
	}
	// Check if right of slice needs to be trimmed (masked)
	if (slices_counter == (slices_max - 1))
		m = 240 - slice_trim;
	else
		m = 240;
	for (i = 122 ; i < m; i++) {
		threshold = spectrum.db[i - 120];			// 240-120 = 120 -> +8 = 128
		if (threshold > xmax) {						// (0~2) 2~120 (120~136) 136~254 (254~256)
			xmax = threshold;
			imax = i - 4;
		}
	}
	slicemax_db[slices_counter] = xmax;
	slicemax_idx[slices_counter] = imax;

	// Add to mean
	for (i = 136; i < 254; i++)
		mean += spectrum.db[i];
	for (i = 2; i < 120; i++)
		mean += spectrum.db[i];
	
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
	rf::Frequency f_min, f_max;
	rf::Frequency slices_span;
	rf::Frequency resolved_frequency;
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
		slice_start = slice_frequency;
		receiver_model.set_tuning_frequency(slice_frequency);
		
		resolved_frequency = (CC_SLICE_WIDTH - scan_span) / 2;		// Trim frequency span (for both sides)
		resolved_frequency /= CC_BIN_WIDTH;							// Convert to bin span
		slice_trim = resolved_frequency;
		
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
	add_children({ {
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
	} });
	
	// DEBUG -------------------------------------------------------------------------
	/*uint8_t testbuffer[] = { 	0xDE, 0x00, 0x02,
								0xCD, 0x00, 0x00,	// Key 0000 = False
									0xC2,
								0xCD, 0x00, 0x01,	// Key 0001 = True
									0xC3,
								0xCD, 0x00, 0x02,	// Key 0002 = False
									0xC2,
								0xCD, 0x00, 0x03,	// Key 0003 = fixnum 19
									19
							};*/
	
	uint8_t testbuffer[100];
	uint8_t debug_v = 7;
	size_t bptr;
	MsgPack msgpack;
	
	bptr = 0;
	
	msgpack.msgpack_init(&testbuffer, &bptr);
	msgpack.msgpack_add(&testbuffer, &bptr, MsgPack::TestListA, false);
	msgpack.msgpack_add(&testbuffer, &bptr, MsgPack::TestListB, true);
	msgpack.msgpack_add(&testbuffer, &bptr, MsgPack::TestListC, false);
	msgpack.msgpack_add(&testbuffer, &bptr, MsgPack::TestListD, (uint8_t)19);
	
	msgpack.msgpack_get(&testbuffer, bptr, MsgPack::TestListD, &debug_v);
	if (debug_v == 19)
		text_debug.set("OK!");
	else
		text_debug.set(to_string_dec_uint(testbuffer[0]));
	
	// DEBUG -------------------------------------------------------------------------
		
	text_labels_a.set_style(&style_grey);
	text_labels_b.set_style(&style_grey);
	text_labels_c.set_style(&style_grey);
	text_slices.set_style(&style_grey);
	text_rate.set_style(&style_grey);
	text_mhz.set_style(&style_grey);
	big_display.set_style(&style_grey);
	
	receiver_model.set_tuning_frequency(467000000);
	
	field_threshold.set_value(80);
	field_threshold.on_change = [this](int32_t v) {
		min_threshold = v;
	};

	field_frequency_min.set_value(receiver_model.tuning_frequency());
	field_frequency_min.set_step(100000);
	field_frequency_min.on_change = [this](rf::Frequency f) {
		(void) f;
		this->on_range_changed();
	};
	field_frequency_min.on_edit = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			this->on_range_changed();
			this->field_frequency_min.set_value(f);
		};
	};
	
	field_frequency_max.set_focusable(false);	// DEBUG
	
	field_frequency_max.set_value(receiver_model.tuning_frequency() + 3000000);
	field_frequency_max.set_step(100000);
	/*field_frequency_max.on_change = [this](rf::Frequency f) {
		(void) f;
		this->on_range_changed();
	};
	field_frequency_max.on_edit = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			this->on_range_changed();
			this->field_frequency_max.set_value(f);
		};
	};*/
	
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
	
	signal_token_tick_second = time::signal_tick_second += [this]() {
		this->on_tick_second();
	};
	receiver_model.set_baseband_bandwidth(CC_SLICE_WIDTH);
	receiver_model.enable();
}

} /* namespace ui */
