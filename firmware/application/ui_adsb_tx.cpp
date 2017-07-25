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

#include "manchester.hpp"
#include "string_format.hpp"
#include "portapack.hpp"
#include "baseband_api.hpp"

#include <cstring>
#include <stdio.h>

using namespace adsb;
using namespace portapack;

namespace ui {

Compass::Compass(
	const Point parent_pos
) : Widget { { parent_pos, { 64, 64 } } }
{
	set_focusable(false);	// Useless ?
}

void Compass::set_value(uint32_t new_value) {
	Point center = screen_pos() + Point(32, 32);
	
	new_value = clamp_value(new_value);
	
	display.draw_line(
		center,
		center + Point(sin_f32(DEG_TO_RAD(value_) + (pi / 2)) * 28, -sin_f32(DEG_TO_RAD(value_)) * 28),
		Color::dark_grey()
	);
	
	display.draw_line(
		center,
		center + Point(sin_f32(DEG_TO_RAD(new_value) + (pi / 2)) * 28, -sin_f32(DEG_TO_RAD(new_value)) * 28),
		Color::green()
	);

	value_ = new_value;
}

void Compass::paint(Painter&) {
	display.fill_circle(screen_pos() + Point(32, 32), 32, Color::dark_grey(), Color::black());
	display.fill_rectangle({ screen_pos() + Point(32 - 2, 0), { 4, 4 } }, Color::black());	// N
	display.fill_rectangle({ screen_pos() + Point(32 - 2, 64 - 4), { 4, 4 } }, Color::black());	// S
	display.fill_rectangle({ screen_pos() + Point(0, 32 - 2), { 4, 4 } }, Color::black());	// W
	display.fill_rectangle({ screen_pos() + Point(64 - 4, 32 - 2), { 4, 4 } }, Color::black());	// E
	
	set_value(value_);
}

uint32_t Compass::clamp_value(uint32_t value) {
	return range.clip(value);
}

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

void ADSBTxView::generate_frames() {
	uint8_t * bin_ptr = shared_memory.bb_data.data;
	
	encode_frame_id(frames[0], sym_icao.value_hex_u64(), callsign);
	
	encode_frame_pos(frames[1], sym_icao.value_hex_u64(), field_altitude.value(),
		field_lat_degrees.value(), field_lon_degrees.value(), 0);
	encode_frame_pos(frames[2], sym_icao.value_hex_u64(), field_altitude.value(),
		field_lat_degrees.value(), field_lon_degrees.value(), 1);

	memset(bin_ptr, 0, 240);

	auto raw_ptr = frames[0].get_raw_data();
	
	// The preamble isn't manchester encoded
	memcpy(bin_ptr, adsb_preamble, 16);
	
	// Convert to binary with manchester encoding (1 byte per bit, faster for baseband code)
	/*for (c = 0; c < 112; c++) {
		if ((raw_ptr[c >> 3] << (c & 7)) & 0x80) {
			bin_ptr[(c * 2) + 16] = 1;
			bin_ptr[(c * 2) + 16 + 1] = 0;
		} else {
			bin_ptr[(c * 2) + 16] = 0;
			bin_ptr[(c * 2) + 16 + 1] = 1;
		}
	}*/
	
	manchester_encode(bin_ptr + 16, raw_ptr, 112, 0);
	
	// Display in hex for debug
	text_frame.set(to_string_hex_array(frames[0].get_raw_data(), 14));

	button_callsign.set_text(callsign);
}

void ADSBTxView::start_tx() {
	generate_frames();
	
	transmitter_model.set_tuning_frequency(434000000);	// DEBUG
	transmitter_model.set_sampling_rate(4000000U);
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_vga(40);
	transmitter_model.set_baseband_bandwidth(10000000);
	transmitter_model.enable();
	
	baseband::set_adsb();
}

void ADSBTxView::on_txdone(const bool v) {
	if (v) {
		transmitter_model.disable();
		tx_view.set_transmitting(false);
	}
}

/*void ADSBTxView::rotate_frames() {
	// DEBUG
	uint8_t * bin_ptr = shared_memory.bb_data.data;
	uint8_t * raw_ptr;
	uint32_t frame_index = 0, plane_index = 0;
	uint32_t c, regen = 0;
	float offs = 0;
	
	for (;;) {
		if (!regen) {
			regen = 10;
			
			encode_frame_id(frames[0], plane_index, "DEMO" + to_string_dec_uint(plane_index));
			encode_frame_pos(frames[1], plane_index, 5000, plane_lats[plane_index]/8 + offs + 38.5, plane_lons[plane_index]/8 + 125.8, 0);
			encode_frame_pos(frames[2], plane_index, 5000, plane_lats[plane_index]/8 + offs + 38.5, plane_lons[plane_index]/8 + 125.8, 1);
			encode_frame_identity(frames[3], plane_index, 1337);
			
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
}*/

ADSBTxView::ADSBTxView(NavigationView& nav) {
	uint32_t c;
	
	baseband::run_image(portapack::spi_flash::image_tag_adsb_tx);

	add_children({
		&labels,
		//&options_format,
		&sym_icao,
		&button_callsign,
		&field_altitude,
		&field_lat_degrees,
		&field_lat_minutes,
		&field_lat_seconds,
		&field_lon_degrees,
		&field_lon_minutes,
		&field_lon_seconds,
		&compass,
		&field_angle,
		&field_speed,
		&check_emergency,
		&field_squawk,
		&text_frame,
		&tx_view
	});
	
	/*options_format.set_by_value(17);	// Mode S
	
	options_format.on_change = [this](size_t, int32_t) {
		generate_frames();
	};*/
	
	sym_icao.on_change = [this]() {
		generate_frames();
	};
	button_callsign.on_select = [this, &nav](Button&) {
		text_prompt(nav, &callsign, 8);
	};
	
	field_altitude.set_value(36000);
	field_lat_degrees.set_value(0);
	field_lat_minutes.set_value(0);
	field_lat_seconds.set_value(0);
	field_lon_degrees.set_value(0);
	field_lon_minutes.set_value(0);
	field_lon_seconds.set_value(0);
	field_angle.set_value(0);
	field_speed.set_value(0);
	
	field_altitude.on_change = [this](int32_t) {
		generate_frames();
	};
	field_lat_degrees.on_change = [this](int32_t) {
		generate_frames();
	};
	field_lon_degrees.on_change = [this](int32_t) {
		generate_frames();
	};
	
	field_angle.on_change = [this](int32_t v) {
		compass.set_value(v);
		generate_frames();
	};
	
	field_speed.on_change = [this](int32_t) {
		generate_frames();
	};
	
	for (c = 0; c < 4; c++)
		field_squawk.set_sym(c, 0);
	
	generate_frames();
	
	receiver_model.set_tuning_frequency(434000000);		// DEBUG
	
	tx_view.on_start = [this]() {
		start_tx();
		tx_view.set_transmitting(true);
		//rotate_frames();
	};
	
	tx_view.on_stop = [this]() {
		tx_view.set_transmitting(false);
		transmitter_model.disable();
	};
}

} /* namespace ui */
