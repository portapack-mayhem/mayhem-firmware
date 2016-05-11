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
#include "evtimer.h"

#include "event_m0.hpp"
#include "ff.h"
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
}

void CloseCallView::do_detection() {
	uint8_t xmax = 0;
	uint16_t imax = 0;
	uint8_t threshold;
	size_t i;
	
	portapack::display.fill_rectangle({last_pos, 60, 1, 9}, Color::black());
	
	mean /= (236 * (slices_max + 1));

	for (i = 0; i < (slices_max + 1); i++) {
		threshold = slicemax_db[i];
		if (threshold >= 40) {
			if (((uint8_t)(threshold - 40) > mean) && (threshold > xmax)) {
				xmax = threshold;
				imax = slicemax_idx[i] + (i * 236);
			}
		}
	}
	
	// Lock / release
	if ((imax >= last_channel - 2) && (imax <= last_channel + 2)) {
		if (detect_counter == 8) {
			if (imax != last_channel) {
				char finalstr[24] = {0};
				
				// 236 steps = 10MHz
				// Resolution = 42.4kHz
				auto fstr = to_string_dec_int((slice_start + (42373 * (imax - 118))) / 1000);

				strcat(finalstr, "Locked: ");
				strcat(finalstr, fstr.c_str());
				text_infos.set(finalstr);
			}
			
			locked = true;
			release_counter = 0;
		} else {
			detect_counter++;
		}
	} else {
		if (locked == true) {
			if (release_counter == 8) {
				detect_counter = 0;
				locked = false;
				text_infos.set("Lost              ");
			} else {
				release_counter++;
			}
		}
	}
	
	last_channel = imax;
	
	last_pos = (ui::Coord)(imax + 2);
	portapack::display.fill_rectangle({last_pos, 60, 1, 9}, Color::red());
}

void CloseCallView::on_channel_spectrum(const ChannelSpectrum& spectrum) {
	uint8_t xmax = 0;
	uint16_t imax = 0;
	uint8_t threshold;
	size_t i;
	
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
			{ { 0, 64 + slices_counter * 4 }, { pixel_row.size(), 1 } },
			pixel_row
		);
		
		// Find max for this slice
		for (i = 0; i < 118; i++) {
			threshold = spectrum.db[256 - 120 + i];
			if (threshold > xmax) {
				xmax = threshold;
				imax = i;
			}
		}
		for (i = 122; i < 240; i++) {
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

	baseband::spectrum_streaming_start();
}

void CloseCallView::on_hide() {
	baseband::spectrum_streaming_stop();

	EventDispatcher::message_map().unregister_handler(Message::ID::DisplayFrameSync);
	EventDispatcher::message_map().unregister_handler(Message::ID::ChannelSpectrumConfig);
}

void CloseCallView::on_range_changed() {
	rf::Frequency f_min, f_max;
	rf::Frequency slices_span;
	int64_t offset;
	
	f_max = field_frequency_max.value();
	f_min = field_frequency_min.value();
	scan_span = abs(f_max - f_min);
	if (scan_span > CC_SLICE_WIDTH) {
		// ex: 100~115 (15): 102.5(97.5~107.5) -> 112.5(107.5~117.5) = 2.5 lost left and right
		slices_max = (scan_span + CC_SLICE_WIDTH - 1) / CC_SLICE_WIDTH;
		// = 2
		slices_span = slices_max * CC_SLICE_WIDTH;
		// = 20000000
		offset = (slices_span - scan_span) / 2;
		// = (20000000 - 15000000) / 2 = 5000000 / 2 = 2500000
		slice_start = std::min(f_min, f_max) + offset;
		
		slices_max--;
		slicing = true;
	} else {
		slice_frequency = (f_max + f_min) / 2;
		//offset = (CC_SLICE_WIDTH - scan_span) / 2;
		//slice_start = std::min(f_min, f_max) - offset;
		slice_start = slice_frequency;
		receiver_model.set_tuning_frequency(slice_frequency);
		slices_max = 0;
		slices_counter = 0;
		slicing = false;
	}
	
	text_slice.set(to_string_dec_int(slices_max + 1));
	slices_counter = 0;
}

CloseCallView::CloseCallView(
	NavigationView& nav
)
{
	add_children({ {
		&field_frequency_min,
		&field_frequency_max,
		&text_slice,
		&text_infos,
		&button_exit
	} });

	field_frequency_min.set_value(receiver_model.tuning_frequency() - 2000000);
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
	field_frequency_max.set_step(200000);
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
	
	on_range_changed();
	
	button_exit.on_select = [&nav](Button&){
		nav.pop();
	};

	/*field_lna.set_value(receiver_model.lna());
	field_lna.on_change = [this](int32_t v) {
		this->on_lna_changed(v);
	};

	field_lna.on_show_options = [this]() {
		this->on_show_options_rf_gain();
	};

	field_vga.set_value(receiver_model.vga());
	field_vga.on_change = [this](int32_t v_db) {
		this->on_vga_changed(v_db);
	};
	field_vga.on_show_options = [this]() {
		this->on_show_options_rf_gain();
	};*/
	
	//receiver_model.set_lna(0);
	//receiver_model.set_vga(0);
	
	
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
