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

#include "ui_adsb_tx.hpp"
#include "ui_alphanum.hpp"

#include "string_format.hpp"
#include "portapack.hpp"
#include "baseband_api.hpp"

#include <cstring>
#include <stdio.h>

using namespace adsb;
using namespace portapack;

namespace ui {

void ADSBTxView::focus() {
	tx_view.focus();
}

ADSBTxView::~ADSBTxView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void ADSBTxView::paint(Painter&) {
	button_callsign.set_text(callsign);
}

void ADSBTxView::generate_frame() {
	uint32_t c;
	uint8_t * bin_ptr = shared_memory.bb_data.data;
	
	generate_frame_id(frames[0], sym_icao.value_hex_u64(), callsign);
	generate_frame_pos(frames[1], sym_icao.value_hex_u64(), 5000, field_lat_degrees.value(), field_lon_degrees.value(), 0);
	generate_frame_pos(frames[2], sym_icao.value_hex_u64(), 5000, field_lat_degrees.value(), field_lon_degrees.value(), 1);

	memset(bin_ptr, 0, 240);

	auto raw_ptr = frames[0].get_raw_data();
	
	memcpy(bin_ptr, adsb_preamble, 16);
	
	// Convert to binary (1 byte per bit, faster for baseband code)
	for (c = 0; c < 112; c++) {
		if ((raw_ptr[c >> 3] << (c & 7)) & 0x80) {
			bin_ptr[(c * 2) + 16] = 1;
			bin_ptr[(c * 2) + 16 + 1] = 0;
		} else {
			bin_ptr[(c * 2) + 16] = 0;
			bin_ptr[(c * 2) + 16 + 1] = 1;
		}
	}
	
	// Display in hex for debug
	text_frame_a.set(to_string_hex_array(frames[0].get_raw_data(), 7));
	text_frame_b.set(to_string_hex_array(frames[0].get_raw_data() + 7, 7));

	button_callsign.set_text(callsign);
}

bool ADSBTxView::start_tx() {
	generate_frame();
	
	transmitter_model.set_tuning_frequency(434000000);
	transmitter_model.set_sampling_rate(4000000U);
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_vga(40);
	transmitter_model.set_baseband_bandwidth(10000000);
	transmitter_model.enable();
	
	baseband::set_adsb();
	
	return true;
}

void ADSBTxView::on_txdone(const bool v) {
	if (v) {
		transmitter_model.disable();
		tx_view.set_transmitting(false);
	}
}

void ADSBTxView::rotate_frames() {
	// DEBUG
	uint8_t * bin_ptr = shared_memory.bb_data.data;
	uint8_t * raw_ptr;
	uint32_t frame_index = 0, plane_index = 0;
	uint32_t c, regen = 0;
	float offs = 0;
	
	for (;;) {
		if (!regen) {
			regen = 10;
			
			generate_frame_id(frames[0], plane_index, "DEMO" + to_string_dec_uint(plane_index));
			generate_frame_pos(frames[1], plane_index, 5000, plane_lats[plane_index]/8 + offs + 38.5, plane_lons[plane_index]/8 + 125.8, 0);
			generate_frame_pos(frames[2], plane_index, 5000, plane_lats[plane_index]/8 + offs + 38.5, plane_lons[plane_index]/8 + 125.8, 1);
			generate_frame_identity(frames[3], plane_index, 1337);
			
			if (plane_index == 11)
				plane_index = 0;
			else
				plane_index++;
			
			offs += 0.001;
		}
		
		memset(bin_ptr, 0, 240);
		
		raw_ptr = frames[frame_index].get_raw_data();
		
		memcpy(bin_ptr, adsb_preamble, 16);
		
		// Convert to binary (1 byte per bit, faster for baseband code)
		for (c = 0; c < 112; c++) {
			if ((raw_ptr[c >> 3] << (c & 7)) & 0x80) {
				bin_ptr[(c * 2) + 16] = 1;
				bin_ptr[(c * 2) + 16 + 1] = 0;
			} else {
				bin_ptr[(c * 2) + 16] = 0;
				bin_ptr[(c * 2) + 16 + 1] = 1;
			}
		}
		
		baseband::set_adsb();
		
		chThdSleepMilliseconds(5);
		
		if (frame_index == 3) {
			frame_index = 0;
			if (regen)
				regen--;
		} else {
			frame_index++;
		}
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
	
	options_format.on_change = [this](size_t, int32_t) {
		generate_frame();
	};
	sym_icao.on_change = [this]() {
		generate_frame();
	};
	button_callsign.on_select = [this, &nav](Button&) {
		text_prompt(nav, &callsign, 8);
	};
	
	field_altitude.set_value(11000);
	field_lat_degrees.set_value(0);
	field_lat_minutes.set_value(0);
	field_lat_seconds.set_value(0);
	field_lon_degrees.set_value(0);
	field_lon_minutes.set_value(0);
	field_lon_seconds.set_value(0);
	
	field_lat_degrees.on_change = [this](int32_t) {
		generate_frame();
	};
	field_lon_degrees.on_change = [this](int32_t) {
		generate_frame();
	};
	
	for (c = 0; c < 4; c++)
		field_squawk.set_sym(c, 0);
	
	generate_frame();
	
	receiver_model.set_tuning_frequency(434000000);		// DEBUG
	
	tx_view.on_start = [this]() {
		if (start_tx())
			tx_view.set_transmitting(true);
		//rotate_frames();
	};
	
	tx_view.on_stop = [this]() {
		tx_view.set_transmitting(false);
		transmitter_model.disable();
	};
}

} /* namespace ui */
