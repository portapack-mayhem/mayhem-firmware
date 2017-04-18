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

#include "ui_whistle.hpp"
#include "ui_receiver.hpp"
#include "tonesets.hpp"

#include "baseband_api.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

namespace ui {

void WhistleView::start_tx() {
	baseband::set_tone(
		0,
		field_tone.value() * TONES_DELTA_COEF,
		(checkbox_stop.value()) ? field_stop.value() * TONES_SAMPLERATE : 0xFFFFFFFF);	// (Almost) infinite duration :)
	
	shared_memory.bb_data.tones_data.message[0] = 0;
	
	transmitter_model.set_sampling_rate(1536000);
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	baseband::set_tones_config(transmitter_model.bandwidth(), 0, 1, false, false);
	
	tx_mode = SINGLE;
}

void WhistleView::on_tx_progress(const bool done) {
	if (done) {
		transmitter_model.disable();
		tx_view.set_transmitting(false);
		tx_mode = IDLE;
	}
}

void WhistleView::focus() {
	tx_view.focus();
}

WhistleView::~WhistleView() {
	transmitter_model.disable();
	baseband::shutdown();
}

WhistleView::WhistleView(
	NavigationView& nav
)
{
	baseband::run_image(portapack::spi_flash::image_tag_tones);
	
	add_children({
		&labels,
		&field_tone,
		&checkbox_stop,
		&field_stop,
		&tx_view
	});
	
	field_tone.set_value(1520);
	field_stop.set_value(15);
	
	tx_view.on_edit_frequency = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			receiver_model.set_tuning_frequency(f);
		};
	};
	
	tx_view.on_start = [this]() {
		if (tx_mode == IDLE) {
			tx_view.set_transmitting(true);
			start_tx();
		}
	};
	
	tx_view.on_stop = [this]() {
		baseband::kill_tone();
	};
}

} /* namespace ui */
