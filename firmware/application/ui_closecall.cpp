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
#include "ui_receiver.hpp"

#include "ch.h"
#include "time.hpp"

#include "event_m0.hpp"
#include "hackrf_gpio.hpp"
#include "portapack.hpp"
#include "radio.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"

#include "hackrf_hal.hpp"

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
	uint16_t imax = 0;
	uint8_t threshold;
	rf::Frequency resolved_frequency;
	size_t i;
	
	portapack::display.fill_rectangle({last_pos, 92, 1, 9}, Color::black());
	
	mean /= (CC_BIN_NB * (slices_max + 1));

	for (i = 0; i < (slices_max + 1); i++) {
		threshold = slicemax_db[i];
		if (threshold >= min_threshold) {
			if (((uint8_t)(threshold - min_threshold) > mean) && (threshold > xmax)) {
				xmax = threshold;
				imax = slicemax_idx[i] + (i * CC_BIN_NB);
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
		if (detect_counter == 8) {
			if (imax != locked_frequency) {
				//char finalstr[24] = {0};
				
				// 236 steps = 5MHz
				// Resolution = 21.2kHz
				resolved_frequency = slice_start + (CC_BIN_WIDTH * (imax - 118));
				//auto fstr = to_string_dec_int(resolved_frequency);

				//strcat(finalstr, "Locked: ");
				//strcat(finalstr, fstr.c_str());
				text_infos.set("Locked !  ");
				big_display.set(resolved_frequency);
				
				locked = true;
				locked_frequency = imax;
			}
			release_counter = 0;
		} else {
			detect_counter++;
		}
	} else {
		detect_counter = 0;
		if (locked == true) {
			if (release_counter == 8) {
				locked = false;
				text_infos.set("Lost      ");
				//big_display.set(0);
			} else {
				release_counter++;
			}
		}
	}
	
	last_channel = imax;
	scan_counter++;
	
	last_pos = (ui::Coord)(imax + 2);
	portapack::display.fill_rectangle({last_pos, 92, 1, 9}, Color::red());
}

void CloseCallView::on_channel_spectrum(const ChannelSpectrum& spectrum) {
	uint8_t xmax = 0;
	uint16_t imax = 0;
	uint8_t threshold;
	size_t i, m;
	
	if (!wait) {
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
		
		// Find max for this slice
		// Check if left of slice needs to be trimmed (masked)
		if (slices_counter == 0)
			i = slice_trim;
		else
			i = 0;
		for ( ; i < 118; i++) {
			threshold = spectrum.db[256 - 120 + i];
			if (threshold > xmax) {
				xmax = threshold;
				imax = i;
			}
		}
		// Check if right of slice needs to be trimmed (masked)
		if (slices_counter == slices_max)
			m = 240 - slice_trim;
		else
			m = 240;
		for (i = 122 ; i < m; i++) {
			threshold = spectrum.db[i - 120];
			if (threshold > xmax) {
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
			if (slices_counter >= slices_max) {
				do_detection();
				mean = 0;
				slices_counter = 0;
			} else {
				slices_counter++;
			}
			slice_frequency = slice_start + (slices_counter * CC_SLICE_WIDTH);
			receiver_model.set_tuning_frequency(slice_frequency);
			wait = 1;
		} else {
			do_detection();
		}
	} else {
		wait--;
	}
}

void CloseCallView::on_show() {
	EventDispatcher::message_map().register_handler(Message::ID::ChannelSpectrumConfig,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const ChannelSpectrumConfigMessage*>(p);
			this->fifo = message.fifo;
		}
	);
	EventDispatcher::message_map().register_handler(Message::ID::DisplayFrameSync,
		[this](const Message* const) {
			if( this->fifo ) {
				ChannelSpectrum channel_spectrum;
				while( fifo->out(channel_spectrum) ) {
					this->on_channel_spectrum(channel_spectrum);
				}
			}
		}
	);

	baseband::spectrum_streaming_start(1);
}

void CloseCallView::on_hide() {
	baseband::spectrum_streaming_stop();

	EventDispatcher::message_map().unregister_handler(Message::ID::DisplayFrameSync);
	EventDispatcher::message_map().unregister_handler(Message::ID::ChannelSpectrumConfig);
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
		offset = (slices_span - scan_span) / 2;
		slice_start = std::min(f_min, f_max) + offset;
		
		slices_max--;	// For slices_counter
		slicing = true;
		
		// Todo: trims
	} else {
		slice_frequency = (f_max + f_min) / 2;
		slice_start = slice_frequency;
		receiver_model.set_tuning_frequency(slice_frequency);
		
		// DEBUG
		resolved_frequency = (CC_SLICE_WIDTH - scan_span) / 2;		// Trim frequency span (for both sides)
		resolved_frequency /= CC_BIN_WIDTH;							// Convert to bin span
		slice_trim = resolved_frequency;
		
		portapack::display.fill_rectangle({0, 97, slice_trim, 4}, Color::orange());
		portapack::display.fill_rectangle({240 - slice_trim, 97, slice_trim, 4}, Color::orange());
		
		slices_max = 0;
		slices_counter = 0;
		slicing = false;
	}
	
	text_debug.set(to_string_dec_uint(slice_start));
	
	text_slices.set(to_string_dec_int(slices_max + 1));
	slices_counter = 0;
}

void CloseCallView::on_lna_changed(int32_t v_db) {
	receiver_model.set_lna(v_db);
}

void CloseCallView::on_vga_changed(int32_t v_db) {
	receiver_model.set_vga(v_db);
}

void CloseCallView::on_tick_second() {
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
		&text_infos,
		&text_debug,
		&big_display,
		&button_exit
	} });
	
	static constexpr Style style_grey {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::grey(),
	};

	text_labels_a.set_style(&style_grey);
	text_labels_b.set_style(&style_grey);
	text_labels_c.set_style(&style_grey);
	text_slices.set_style(&style_grey);
	text_rate.set_style(&style_grey);
	
	field_threshold.set_value(80);
	field_threshold.on_change = [this](int32_t v) {
		min_threshold = v;
	};

	field_frequency_min.set_value(receiver_model.tuning_frequency());
	field_frequency_min.set_step(200000);
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
	
	field_frequency_max.set_value(receiver_model.tuning_frequency() + 2000000);
	field_frequency_max.set_step(100000);
	field_frequency_max.on_change = [this](rf::Frequency f) {
		(void) f;
		this->on_range_changed();
	};
	field_frequency_max.on_edit = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			this->on_range_changed();
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
	
	signal_token_tick_second = time::signal_tick_second += [this]() {
		this->on_tick_second();
	};
	
	//audio::output::mute();
	receiver_model.set_baseband_configuration({
		.mode = toUType(ReceiverModel::Mode::CloseCall),
		.sampling_rate = CC_SLICE_WIDTH,
		.decimation_factor = 1,
	});
	receiver_model.set_baseband_bandwidth(CC_SLICE_WIDTH);
	receiver_model.enable();
	//audio::output::unmute();

}

} /* namespace ui */
