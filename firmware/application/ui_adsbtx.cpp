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

#include "ui_adsbtx.hpp"
#include "ui_alphanum.hpp"

#include "adsb.hpp"
#include "string_format.hpp"
#include "portapack.hpp"
#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"

#include <cstring>
#include <stdio.h>

using namespace adsb;
using namespace portapack;

namespace ui {

void ADSBTxView::focus() {
	//sym_icao.focus();
	tx_view.focus();
}

ADSBTxView::~ADSBTxView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void ADSBTxView::paint(Painter& painter) {
	(void)painter;
	button_callsign.set_text(callsign);
}

void ADSBTxView::generate_frame() {
	uint32_t c;
	std::string str_debug;
	
	generate_frame_id(adsb_frame, sym_icao.value_hex_u64(), callsign);

	memset(adsb_bin, 0, 112);

	// Convert to binary (1 byte per bit, faster for baseband code)
	for (c = 0; c < 112; c++) {
		if ((adsb_frame[c >> 3] << (c & 7)) & 0x80)
			adsb_bin[c] = 1;
	}
	
	// Display in hex for debug
	text_frame_a.set(to_string_hex_array(&adsb_frame[0], 7));
	text_frame_b.set(to_string_hex_array(&adsb_frame[7], 7));

	button_callsign.set_text(callsign);
}

bool ADSBTxView::start_tx() {
	generate_frame();
	
	memcpy(shared_memory.bb_data.data, adsb_bin, 112);
	baseband::set_adsb();
	
	transmitter_model.set_tuning_frequency(1090000000);		// FOR TESTING - DEBUG
	transmitter_model.set_sampling_rate(4000000U);
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_vga(40);
	transmitter_model.set_baseband_bandwidth(2500000);
	transmitter_model.enable();
	
	return false;	// DEBUG
}

void ADSBTxView::on_txdone(const bool v) {
	//size_t sr;

	if (v) {
		transmitter_model.disable();
		tx_view.set_transmitting(false);
		//progress.set_value(0);
	} else {
		//progress.set_value(n);
	}
}

ADSBTxView::ADSBTxView(NavigationView& nav) {
	uint32_t c;
	
	baseband::run_image(portapack::spi_flash::image_tag_adsb_tx);

	add_children({
		&labels,
		&options_format,
		&sym_icao,
		&button_callsign,
		&field_altitude,
		&field_lat_degrees,
		&field_lat_minutes,
		&field_lat_seconds,
		&field_lon_degrees,
		&field_lon_minutes,
		&field_lon_seconds,
		&check_emergency,
		&field_squawk,
		&text_frame_a,
		&text_frame_b,
		&tx_view
	});
	
	options_format.set_by_value(17);	// Mode S
	
	options_format.on_change = [this](size_t i, int32_t v) {
		(void)i;
		(void)v;
		generate_frame();
	};
	sym_icao.on_change = [this]() {
		generate_frame();
	};
	button_callsign.on_select = [this, &nav](Button&) {
		text_entry(nav, &callsign, 9);
	};
	
	field_altitude.set_value(11000);
	field_lat_degrees.set_value(0);
	field_lat_minutes.set_value(0);
	field_lat_seconds.set_value(0);
	field_lon_degrees.set_value(0);
	field_lon_minutes.set_value(0);
	field_lon_seconds.set_value(0);
	
	for (c = 0; c < 4; c++)
		field_squawk.set_sym(c, 0);
	
	generate_frame();
	
	tx_view.on_edit_frequency = [this, &nav]() {
		// Shouldn't be able to edit frequency
		return;
	};
	
	tx_view.on_start = [this]() {
		if (start_tx())
			tx_view.set_transmitting(true);
	};
	
	tx_view.on_stop = [this]() {
		tx_view.set_transmitting(false);
		transmitter_model.disable();
	};
}

} /* namespace ui */
