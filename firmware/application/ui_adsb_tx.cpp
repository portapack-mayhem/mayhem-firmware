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
#include "ui_geomap.hpp"

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
}

Point Compass::polar_to_point(uint32_t angle, uint32_t distance) {
	return Point(sin_f32(DEG_TO_RAD(angle) + (pi / 2)) * distance, -sin_f32(DEG_TO_RAD(angle)) * distance);
}

void Compass::set_value(uint32_t new_value) {
	Point center = screen_pos() + Point(32, 32);
	
	new_value = range.clip(new_value);
	
	display.draw_line(
		center,
		center + polar_to_point(value_, 28),
		Color::dark_grey()
	);
	
	display.draw_line(
		center,
		center + polar_to_point(new_value, 28),
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

ADSBView::ADSBView() {
	add_child(&check_enable);
	hidden(true);
	
	check_enable.on_select = [this](Checkbox&, bool value) {
		enabled = value;
	};
}

void ADSBView::set_enabled(bool value) {
	check_enable.set_value(value);
}

void ADSBView::set_type(std::string type) {
	check_enable.set_text("Transmit " + type);
}

void ADSBView::focus() {
	check_enable.focus();
}

ADSBPositionView::ADSBPositionView(NavigationView& nav) {
	set_type("position");
	
	add_children({
		&labels_position,
		&field_altitude,
		&field_lat_degrees,
		&field_lat_minutes,
		&field_lat_seconds,
		&field_lon_degrees,
		&field_lon_minutes,
		&field_lon_seconds,
		&button_set_map
	});
	
	field_altitude.set_value(36000);
	field_lat_degrees.set_value(0);
	field_lat_minutes.set_value(0);
	field_lat_seconds.set_value(0);
	field_lon_degrees.set_value(0);
	field_lon_minutes.set_value(0);
	field_lon_seconds.set_value(0);
	
	button_set_map.on_select = [this, &nav](Button&) {
		nav.push<GeoMapView>(GeoMapView::Mode::SET);
	};
}

void ADSBPositionView::collect_frames(const uint32_t ICAO_address, std::vector<ADSBFrame>& frame_list) {
	ADSBFrame temp_frame;
	
	encode_frame_pos(temp_frame, ICAO_address, field_altitude.value(),
		field_lat_degrees.value(), field_lon_degrees.value(), 0);
	
	frame_list.emplace_back(temp_frame);
	
	encode_frame_pos(temp_frame, ICAO_address, field_altitude.value(),
		field_lat_degrees.value(), field_lon_degrees.value(), 1);
		
	frame_list.emplace_back(temp_frame);
}

ADSBCallsignView::ADSBCallsignView(NavigationView& nav) {
	set_type("callsign");
	
	add_children({
		&labels_callsign,
		&button_callsign
	});
	
	set_enabled(true);
	
	button_callsign.set_text(callsign);
	
	button_callsign.on_select = [this, &nav](Button&) {
		text_prompt(nav, &callsign, 8);
	};
}

void ADSBCallsignView::collect_frames(const uint32_t ICAO_address, std::vector<ADSBFrame>& frame_list) {
	ADSBFrame temp_frame;
	
	encode_frame_id(temp_frame, ICAO_address, callsign);
	
	frame_list.emplace_back(temp_frame);
}

ADSBSpeedView::ADSBSpeedView() {
	set_type("speed");
	
	add_children({
		&labels_speed,
		&compass,
		&field_angle,
		&field_speed
	});
	
	field_angle.set_value(0);
	field_speed.set_value(400);
	
	field_angle.on_change = [this](int32_t v) {
		compass.set_value(v);
	};
}

void ADSBSpeedView::collect_frames(const uint32_t ICAO_address, std::vector<ADSBFrame>& frame_list) {
	ADSBFrame temp_frame;
	
	encode_frame_velo(temp_frame, ICAO_address, field_speed.value(),
		field_angle.value(), 0);	// TODO: v_rate
	
	frame_list.emplace_back(temp_frame);
}

ADSBSquawkView::ADSBSquawkView() {
	set_type("squawk");
	
	add_children({
		&labels_squawk,
		&field_squawk
	});
}

void ADSBSquawkView::collect_frames(const uint32_t ICAO_address, std::vector<ADSBFrame>& frame_list) {
	ADSBFrame temp_frame;
	(void)ICAO_address;
	
	encode_frame_squawk(temp_frame, field_squawk.value_dec_u32());
	
	frame_list.emplace_back(temp_frame);
}

void ADSBTxView::focus() {
	tab_view.focus();
}

ADSBTxView::~ADSBTxView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void ADSBTxView::generate_frames() {
	const uint32_t ICAO_address = sym_icao.value_hex_u64();
	
	view_position.collect_frames(ICAO_address, frames);
	view_callsign.collect_frames(ICAO_address, frames);
	view_speed.collect_frames(ICAO_address, frames);
	view_squawk.collect_frames(ICAO_address, frames);

	//memset(bin_ptr, 0, 240);

	//auto raw_ptr = frames[0].get_raw_data();
	
	// The preamble isn't manchester encoded
	//memcpy(bin_ptr, adsb_preamble, 16);
	
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
	
	/*manchester_encode(bin_ptr + 16, raw_ptr, 112, 0);
	
	// Display in hex for debug
	text_frame.set(to_string_hex_array(frames[0].get_raw_data(), 14));

	button_callsign.set_text(callsign);*/
}

void ADSBTxView::start_tx() {
	generate_frames();
	
	transmitter_model.set_sampling_rate(4000000U);
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_baseband_bandwidth(10000000);
	transmitter_model.enable();
	
	baseband::set_adsb();
}

void ADSBTxView::on_txdone(const bool v) {
	(void)v;
	/*if (v) {
		transmitter_model.disable();
		tx_view.set_transmitting(false);
	}*/
}

void ADSBTxView::rotate_frames() {
	uint8_t * bin_ptr = shared_memory.bb_data.data;
	uint8_t * raw_ptr;
	uint32_t frame_index = 0;	//, plane_index = 0;
	uint32_t c;	//, regen = 0;
	//float offs = 0;
	
	for (;;) {
		/*if (!regen) {
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
		}*/
		
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
		
		chThdSleepMilliseconds(50);
		
		if (frame_index == frames.size()) {
			frame_index = 0;
			//if (regen)
			//	regen--;
		} else {
			frame_index++;
		}
	}
}

ADSBTxView::ADSBTxView(
	NavigationView& nav
) : nav_ { nav }
{
	Rect view_rect = { 0, 7 * 8, 240, 192 };
	baseband::run_image(portapack::spi_flash::image_tag_adsb_tx);

	add_children({
		&tab_view,
		&labels,
		&sym_icao,
		&view_position,
		&view_callsign,
		&view_speed,
		&view_squawk,
		&text_frame,
		&tx_view
	});
	
	view_position.set_parent_rect(view_rect);
	view_callsign.set_parent_rect(view_rect);
	view_speed.set_parent_rect(view_rect);
	view_squawk.set_parent_rect(view_rect);
	
	sym_icao.on_change = [this]() {
		generate_frames();
	};
	
	tx_view.on_start = [this]() {
		start_tx();
		tx_view.set_transmitting(true);
		rotate_frames();
	};
	
	tx_view.on_stop = [this]() {
		tx_view.set_transmitting(false);
		transmitter_model.disable();
	};
}

} /* namespace ui */
