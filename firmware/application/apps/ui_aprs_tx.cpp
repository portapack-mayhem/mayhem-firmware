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

#include "ui_aprs_tx.hpp"
#include "ui_alphanum.hpp"

#include "aprs.hpp"
#include "string_format.hpp"
#include "portapack.hpp"
#include "baseband_api.hpp"
#include "portapack_shared_memory.hpp"
#include "portapack_persistent_memory.hpp"

#include <cstring>
#include <stdio.h>

using namespace aprs;
using namespace portapack;

namespace ui {

void APRSTXView::focus() {
	tx_view.focus();
}

APRSTXView::~APRSTXView() {
	// save app settings
	app_settings.tx_frequency = transmitter_model.tuning_frequency();	
	settings.save("tx_aprs", &app_settings);

	transmitter_model.disable();
	baseband::shutdown();
}

void APRSTXView::start_tx() {
	make_aprs_frame(
		sym_source.value_string().c_str(), num_ssid_source.value(),
		sym_dest.value_string().c_str(), num_ssid_dest.value(),
		payload);
	
	//uint8_t * bb_data_ptr = shared_memory.bb_data.data;
	//text_payload.set(to_string_hex_array(bb_data_ptr + 56, 15));
	
	transmitter_model.set_tuning_frequency(persistent_memory::tuned_frequency());
	transmitter_model.set_sampling_rate(AFSK_TX_SAMPLERATE);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	baseband::set_afsk_data(
		AFSK_TX_SAMPLERATE / 1200,
		1200,
		2200,
		1,
		10000,	//APRS uses fixed 10k bandwidth
		8
	);
}

void APRSTXView::on_tx_progress(const uint32_t progress, const bool done) {
	(void)progress;
	
	if (done) {
		transmitter_model.disable();
		tx_view.set_transmitting(false);
	}
}

APRSTXView::APRSTXView(NavigationView& nav) {
	
	baseband::run_image(portapack::spi_flash::image_tag_afsk);

	add_children({
		&labels,
		&sym_source,
		&num_ssid_source,
		&sym_dest,
		&num_ssid_dest,
		&text_payload,
		&button_set,
		&tx_view
	});
	

	// load app settings
	auto rc = settings.load("tx_aprs", &app_settings);
	if(rc == SETTINGS_OK) {
		transmitter_model.set_rf_amp(app_settings.tx_amp);
		transmitter_model.set_tuning_frequency(app_settings.tx_frequency);
		transmitter_model.set_tx_gain(app_settings.tx_gain);		
	}

	button_set.on_select = [this, &nav](Button&) {
		text_prompt(
			nav,
			payload,
			30,
			[this](std::string& s) {
				text_payload.set(s);
			}
		);
	};
	
	tx_view.on_edit_frequency = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			receiver_model.set_tuning_frequency(f);
		};
	};
	
	tx_view.on_start = [this]() {
		start_tx();
		tx_view.set_transmitting(true);
	};
	
	tx_view.on_stop = [this]() {
		tx_view.set_transmitting(false);
		transmitter_model.disable();
	};
}

} /* namespace ui */
