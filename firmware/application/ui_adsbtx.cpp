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
	button_transmit.focus();
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
	uint8_t c;
	std::string str_debug;

	if (options_format.selected_index() == 2)
		button_transmit.hidden(true);
	
	generate_frame_id(adsb_frame, sym_icao.value_hex_u64(), callsign);

	memset(adsb_bin, 0, 112);

	// Convert to binary (1 bit per byte, faster for baseband code)
	for (c = 0; c < 112; c++) {
		if ((adsb_frame[c >> 3] << (c & 7)) & 0x80) {
			adsb_bin[c] = 0xFF;
		}
	}
	
	// Display for debug
	str_debug = "";
	for (c = 0; c < 7; c++)
		str_debug += to_string_hex(adsb_frame[c], 2);
	text_frame_a.set(str_debug);
	str_debug = "";
	for (c = 0; c < 7; c++)
		str_debug += to_string_hex(adsb_frame[c + 7], 2);
	text_frame_b.set(str_debug);

	//text_message.set(callsign_formatted);
}

void ADSBTxView::start_tx() {
	transmitter_model.set_tuning_frequency(452000000);		// FOR TESTING - DEBUG
	transmitter_model.set_baseband_configuration({
		.mode = 0,
		.sampling_rate = 2000000U,		// Good ?
		.decimation_factor = 1,
	});
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_lna(40);
	transmitter_model.set_vga(40);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	memcpy(shared_memory.tx_data, adsb_bin, 112);
	baseband::set_adsb();
}

void ADSBTxView::on_txdone(const int n) {
	size_t sr;

	if (n == 200) {
		transmitter_model.disable();
		//progress.set_value(0);
		
		tx_mode = IDLE;
		button_transmit.set_style(&style_val);
		button_transmit.set_text("START");
	} else {
		//progress.set_value(n);
	}
}

ADSBTxView::ADSBTxView(NavigationView& nav) {
	(void)nav;
	
	baseband::run_image(portapack::spi_flash::image_tag_adsb_tx);

	// http://openflights.org

	add_children({ {
		&text_format,
		&options_format,
		&text_icaolabel,
		&sym_icao,
		&text_callsign,
		&button_callsign,
		&text_altitude,
		&field_altitude,
		&text_latitude,
		&field_lat_degrees,
		&field_lat_minutes,
		&field_lat_seconds,
		&text_longitude,
		&field_lon_degrees,
		&field_lon_minutes,
		&field_lon_seconds,
		&text_frame_a,
		&text_frame_b,
		&button_transmit
	} });
	
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
		textentry(nav, callsign, 9);
	};
	
	field_altitude.set_value(11000);
	field_lat_degrees.set_value(0);
	field_lat_minutes.set_value(0);
	field_lat_seconds.set_value(0);
	field_lon_degrees.set_value(0);
	field_lon_minutes.set_value(0);
	field_lon_seconds.set_value(0);
	
	button_transmit.set_style(&style_val);
	
	generate_frame();

	// Single transmit
	button_transmit.on_select = [this, &nav](Button&) {
		if (tx_mode == IDLE) {
			tx_mode = SINGLE;
			button_transmit.set_style(&style_cancel);
			button_transmit.set_text("Wait");
			generate_frame();
			start_tx();
		}
	};
}

} /* namespace ui */
