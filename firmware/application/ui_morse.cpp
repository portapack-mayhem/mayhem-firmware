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

/*
Keying speed: 60 or 75 PARIS
Continuous (Fox-oring)
12s transmit, 48s space (Sprint 1/5th) 
60s transmit, 240s space (Classic 1/5 min) 
60s transmit, 360s space (Classic 1/7 min) 
*/

#include "ui_morse.hpp"

#include "portapack.hpp"
#include "baseband_api.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;
using namespace morse;

// TODO: Live keying mode: Dit on left key, dah on right ?

namespace ui {

void MorseView::focus() {
	tx_view.focus();
}

MorseView::~MorseView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void MorseView::paint(Painter& painter) {
	(void)painter;
}

bool MorseView::start_tx() {
	size_t symbol_count;
	std::string message;
	
	if (checkbox_foxhunt.value()) {
		message = foxhunt_codes[options_foxhunt.selected_index_value()];
	} else {
		message = "ABCDEFGHIJKLMN";
	}
	
	symbol_count = morse_encode(message, field_time_unit.value(), field_tone.value());
	
	if (!symbol_count) {
		nav_.display_modal("Error", "Message too long.", INFO, nullptr);
		return false;
	}
	
	progressbar.set_max(symbol_count);
	
	transmitter_model.set_sampling_rate(1536000U);
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_lna(40);
	transmitter_model.set_vga(40);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	baseband::set_tones_data(transmitter_model.bandwidth(), 0, symbol_count, false, false);
	
	return true;
}

void MorseView::on_tx_progress(const int progress, const bool done) {
	if (done) {
		transmitter_model.disable();
		progressbar.set_value(0);
		tx_view.set_transmitting(false);
	} else
		progressbar.set_value(progress);
}

MorseView::MorseView(
	NavigationView& nav
) : nav_ (nav)
{
	baseband::run_image(portapack::spi_flash::image_tag_tones);
	
	add_children({
		&checkbox_foxhunt,
		&options_foxhunt,
		&text_time_unit,
		&field_time_unit,
		&text_tone,
		&field_tone,
		&progressbar,
		&tx_view
	});
	
	field_time_unit.set_value(50);		// 50ms
	field_tone.set_value(700);			// 700Hz
	
	tx_view.on_edit_frequency = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			receiver_model.set_tuning_frequency(f);
		};
	};
	
	tx_view.on_start = [this]() {
		if (start_tx())
			tx_view.set_transmitting(true);
	};
	
	tx_view.on_stop = [this]() {
		tx_view.set_transmitting(false);
	};

}

} /* namespace ui */
