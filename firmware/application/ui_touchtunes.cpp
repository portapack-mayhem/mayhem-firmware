/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#include "ui_touchtunes.hpp"
#include "encoders.hpp"

#include "baseband_api.hpp"
#include "string_format.hpp"

using namespace portapack;
using namespace encoders;

namespace ui {

void TouchTunesView::focus() {
	field_pin.focus();
}

TouchTunesView::~TouchTunesView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void TouchTunesView::on_tx_progress(const int progress, const bool done) {
	if (!done) {
		// Repeating
		if (tx_mode == SINGLE) {
			progressbar.set_value(progress);
		} else if (tx_mode == SCAN) {
			progressbar.set_value((pin * 4) + progress);
		}
	} else {
		// Done transmitting
		if (tx_mode == SINGLE) {
			transmitter_model.disable();
			tx_mode = IDLE;
			progressbar.set_value(0);
		} else if (tx_mode == SCAN) {
			pin++;
			if (pin == 256) {
				transmitter_model.disable();
				tx_mode = IDLE;
				progressbar.set_value(0);
			} else {
				field_pin.set_value(pin);
				start_tx(scan_button_index);
			}
		}
	}
}

void TouchTunesView::start_tx(const uint32_t button_index) {
	std::string fragments;
	size_t bit;
	uint64_t frame_data { 0 };
	
	if (check_scan.value()) {
		scan_button_index = button_index;
		tx_mode = SCAN;
		progressbar.set_max(256 * 4);
	} else {
		tx_mode = SINGLE;
		progressbar.set_max(4);
	}
	
	frame_data = 0x5D;		// Sync word
	
	for (bit = 0; bit < 8; bit++) {
		frame_data <<= 1;
		if (pin & (1 << bit))
			frame_data |= 1;
	}
	
	frame_data <<= 16;
	frame_data |= button_codes[button_index];
	
	for (bit = 0; bit < (8 + 8 + 16); bit++) {
		fragments += (frame_data & 0x80000000UL) ? "1000" : "10";
		frame_data <<= 1;
	}
	fragments = "111111111111111100000000" + fragments + "1000";	// End pulse
	
	size_t bitstream_length = make_bitstream(fragments);
	
	transmitter_model.set_tuning_frequency(433920000);
	transmitter_model.set_sampling_rate(OOK_SAMPLERATE);
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	baseband::set_ook_data(
		bitstream_length,
		OOK_SAMPLERATE / 1766,	// 1766 baud, 566us/bit
		4,						// Repeats
		100
	);
}

TouchTunesView::TouchTunesView(
	NavigationView&
)
{
	baseband::run_image(portapack::spi_flash::image_tag_ook);

	add_children({
		&labels,
		&field_pin,
		&button_on_off,
		&button_pause,
		&button_p1,
		&button_ok,
		&button_vol_inc1,
		&button_vol_inc2,
		&button_vol_inc3,
		&check_scan,
		&text_status,
		&progressbar
	});
	
	field_pin.set_value(pin);
	
	field_pin.on_change = [this](int32_t v) {
		pin = v;
	};
	
	button_on_off.on_select = [this](Button&) {
		start_tx(0);
	};
	
	button_pause.on_select = [this](Button&) {
		start_tx(1);
	};
	button_p1.on_select = [this](Button&) {
		start_tx(2);
	};
	button_ok.on_select = [this](Button&) {
		start_tx(3);
	};
	button_vol_inc1.on_select = [this](Button&) {
		start_tx(4);
	};
	button_vol_inc2.on_select = [this](Button&) {
		start_tx(5);
	};
	button_vol_inc3.on_select = [this](Button&) {
		start_tx(6);
	};
}

} /* namespace ui */
