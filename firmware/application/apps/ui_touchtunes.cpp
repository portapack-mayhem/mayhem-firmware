/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
 * Copyright (C) 2022 NotPike
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

void TouchTunesView::stop_tx() {
	transmitter_model.disable();
	tx_mode = IDLE;
	progressbar.set_value(0);

	// EW Mode Check
	if(check_ew.value()) {
		start_ew();
	} else {
 		text_status.set("Ready");
	}
}

void TouchTunesView::on_tx_progress(const uint32_t progress, const bool done) {
	if (!done) {
		// Progress
		if (tx_mode == SINGLE)
			progressbar.set_value(progress);
		else if (tx_mode == SCAN)
			progressbar.set_value((pin * TOUCHTUNES_REPEATS) + progress);
	} else {
		// Done transmitting
		if (tx_mode == SINGLE) {
			stop_tx();
		} else if (tx_mode == SCAN) {
			if (pin == TOUCHTUNES_MAX_PIN) {
				stop_tx();
			} else {
				transmitter_model.disable();
				pin++;
				field_pin.set_value(pin);
				start_tx(scan_button_index);
			}
		}
	}
}

// EW (Electronic Warfare) Mode will jam the receiving jukebox
// while still alowing you (the hacker) to send commands
// to the target jukebox.
// EW Mode works by transmitting a CW on 433.92MHz inbetween
// transmission events.
void TouchTunesView::start_ew() {
	// Radio
	transmitter_model.set_tuning_frequency(433920000);
	transmitter_model.set_sampling_rate(3072000U);
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_baseband_bandwidth(3500000U);
	transmitter_model.set_tx_gain(47);
	transmitter_model.enable();

	//UI
	text_status.set("Jamming...");
	progressbar.set_max(1);
	progressbar.set_value(1);

}

void TouchTunesView::stop_ew() {
	// Radio
	transmitter_model.disable();

	// UI
	text_status.set("Ready");
	progressbar.set_value(0);
}

void TouchTunesView::start_tx(const uint32_t button_index) {

	// Check EW Mode
	if(check_ew.value()) {
		stop_ew();
	}

	std::string fragments = { "" };
	size_t bit;
	uint64_t frame_data;
	
	if (check_scan.value()) {
		scan_button_index = button_index;
		tx_mode = SCAN;
		progressbar.set_max(TOUCHTUNES_MAX_PIN * TOUCHTUNES_REPEATS);
		text_status.set("Scanning...");
	} else {
		tx_mode = SINGLE;
		progressbar.set_max(TOUCHTUNES_REPEATS);
		text_status.set("Transmitting...");
	}
	
	frame_data = TOUCHTUNES_SYNC_WORD;		// Sync word
	
	// Insert pin value (LSB first)
	for (bit = 0; bit < 8; bit++) {
		frame_data <<= 1;
		if (pin & (1 << bit))
			frame_data |= 1;
	}
	
	// Insert button code (and its complement)
	frame_data <<= 16;
	frame_data |= (button_codes[button_index] << 8);
	frame_data |= (button_codes[button_index] ^ 0xFF);
	
	// Convert to OOK symbols
	for (bit = 0; bit < (8 + 8 + 16); bit++) {
		fragments += (frame_data & 0x80000000UL) ? "1000" : "10";
		frame_data <<= 1;
	}
	
	// Sync and end pulse
	fragments = "111111111111111100000000" + fragments + "1000";
	
	size_t bitstream_length = make_bitstream(fragments);
	
	transmitter_model.set_tuning_frequency(433920000);
	transmitter_model.set_sampling_rate(OOK_SAMPLERATE);
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	baseband::set_ook_data(
		bitstream_length,
		OOK_SAMPLERATE / 1766,	// 560us
		TOUCHTUNES_REPEATS,
		100						// Pause
	);
}

TouchTunesView::TouchTunesView(
	NavigationView&
) {
	baseband::run_image(portapack::spi_flash::image_tag_ook);

	add_children({
		&labels,
		&field_pin,
		&check_scan,
		&check_ew,
		&text_status,
		&progressbar
	});
	
	field_pin.set_value(pin);
	
	field_pin.on_change = [this](int32_t v) {
		pin = v;
	};

	// EW Mode
	check_ew.on_select = [this](Checkbox&, bool v) {
		if(v){
			start_ew();
		} else {
			stop_ew();
		}
	};
	
	const auto button_fn = [this](Button& button) {
		start_tx(button.id);
	};
	
	size_t n = 0;
	for (auto& entry : remote_layout) {
		buttons[n].on_select = button_fn;
		buttons[n].id = n;
		buttons[n].set_text(entry.text);
		buttons[n].set_parent_rect({
			entry.position + Point(8, 0),
			{ (Dim)(entry.text.length() + 2) * 8, 4 * 8 }
		});
		add_child(&buttons[n]);
		n++;
	}
}

} /* namespace ui */
