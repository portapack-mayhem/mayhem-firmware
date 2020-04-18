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

ADSBPositionView::ADSBPositionView(
	NavigationView& nav, Rect parent_rect
) : OptionTabView(parent_rect)
{
	set_type("position");
	
	add_children({
		&geopos,
		&button_set_map
	});
	
	geopos.set_altitude(36000);
	
	button_set_map.on_select = [this, &nav](Button&) {
		nav.push<GeoMapView>(
			geopos.altitude(),
			GeoPos::alt_unit::FEET,
			geopos.lat(),
			geopos.lon(),
			[this](int32_t altitude, float lat, float lon) {
				geopos.set_altitude(altitude);
				geopos.set_lat(lat);
				geopos.set_lon(lon);
			});
	};
}

void ADSBPositionView::collect_frames(const uint32_t ICAO_address, std::vector<ADSBFrame>& frame_list) {
	if (!enabled) return;
	
	ADSBFrame temp_frame;
	
	encode_frame_pos(temp_frame, ICAO_address, geopos.altitude(),
		geopos.lat(), geopos.lon(), 0);
	
	frame_list.emplace_back(temp_frame);
	
	encode_frame_pos(temp_frame, ICAO_address, geopos.altitude(),
		geopos.lat(), geopos.lon(), 1);
		
	frame_list.emplace_back(temp_frame);
}

ADSBCallsignView::ADSBCallsignView(
	NavigationView& nav, Rect parent_rect
) : OptionTabView(parent_rect)
{
	set_type("callsign");
	
	add_children({
		&labels_callsign,
		&button_callsign
	});
	
	set_enabled(true);
	
	button_callsign.set_text(callsign);
	
	button_callsign.on_select = [this, &nav](Button&) {
		text_prompt(
			nav,
			callsign,
			8,
			[this](std::string& s) {
				button_callsign.set_text(s);
			}
		);
	};
}

void ADSBCallsignView::collect_frames(const uint32_t ICAO_address, std::vector<ADSBFrame>& frame_list) {
	if (!enabled) return;
	
	ADSBFrame temp_frame;
	
	encode_frame_id(temp_frame, ICAO_address, callsign);
	
	frame_list.emplace_back(temp_frame);
}

ADSBSpeedView::ADSBSpeedView(
	Rect parent_rect
) : OptionTabView(parent_rect)
{
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
	if (!enabled) return;
	
	ADSBFrame temp_frame;
	
	encode_frame_velo(temp_frame, ICAO_address, field_speed.value(),
		field_angle.value(), 0);	// TODO: v_rate
	
	frame_list.emplace_back(temp_frame);
}

ADSBSquawkView::ADSBSquawkView(
	Rect parent_rect
) : OptionTabView(parent_rect)
{
	set_type("squawk");
	
	add_children({
		&labels_squawk,
		&field_squawk
	});
}

void ADSBSquawkView::collect_frames(const uint32_t ICAO_address, std::vector<ADSBFrame>& frame_list) {
	if (!enabled) return;
	
	ADSBFrame temp_frame;
	(void)ICAO_address;
	
	encode_frame_squawk(temp_frame, field_squawk.value_dec_u32());
	
	frame_list.emplace_back(temp_frame);
}

ADSBTXThread::ADSBTXThread(
	std::vector<ADSBFrame> frames
) : frames_ {  std::move(frames) }
{
	thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO + 10, ADSBTXThread::static_fn, this);
}

ADSBTXThread::~ADSBTXThread() {
	if( thread ) {
		chThdTerminate(thread);
		chThdWait(thread);
		thread = nullptr;
	}
}

msg_t ADSBTXThread::static_fn(void* arg) {
	auto obj = static_cast<ADSBTXThread*>(arg);
	obj->run();
	return 0;
}

void ADSBTXThread::run() {
	uint8_t * bin_ptr = shared_memory.bb_data.data;
	uint8_t * raw_ptr;
	uint32_t frame_index = 0;	//, plane_index = 0;
	//uint32_t regen = 0;
	//float offs = 0;
	
	while( !chThdShouldTerminate() ) {
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
		
		memset(bin_ptr, 0, 256);	// 112 bits * 2 parts = 224 should be enough
		
		raw_ptr = frames_[frame_index].get_raw_data();
		
		// The preamble isn't manchester encoded
		memcpy(bin_ptr, adsb_preamble, 16);
		
		// Convert to binary (1 byte per bit, faster for baseband code)
		manchester_encode(bin_ptr + 16, raw_ptr, 112, 0);
	
		// Display in hex for debug
		//text_frame.set(to_string_hex_array(frames[0].get_raw_data(), 14));
	
		baseband::set_adsb();
		
		chThdSleepMilliseconds(50);
		
		frame_index++;
		if (frame_index >= frames_.size()) {
			frame_index = 0;
			//if (regen)
			//	regen--;
		}
	}
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
	
	frames.clear();
	
	/* This scheme kinda sucks. Each "tab"'s collect_frames method
	 * is called to generate its related frame(s). Getting values
	 * from each widget of each tab would be better ?
	 * */
	view_position.collect_frames(ICAO_address, frames);
	view_callsign.collect_frames(ICAO_address, frames);
	view_speed.collect_frames(ICAO_address, frames);
	view_squawk.collect_frames(ICAO_address, frames);
	
	// Show how many frames were generated
	//text_frame.set(to_string_dec_uint(frames.size()) + " frame(s).");
}

void ADSBTxView::start_tx() {
	generate_frames();
	
	transmitter_model.set_sampling_rate(4000000U);
	transmitter_model.set_baseband_bandwidth(10000000);
	transmitter_model.enable();
	
	baseband::set_adsb();
	
	tx_thread = std::make_unique<ADSBTXThread>(frames);
}

ADSBTxView::ADSBTxView(
	NavigationView& nav
) : nav_ { nav }
{
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
	
	tx_view.on_edit_frequency = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			transmitter_model.set_tuning_frequency(f);
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
