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

#include "ui_adsb_rx.hpp"
#include "ui_alphanum.hpp"
#include "ui_geomap.hpp"

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

template<>
void RecentEntriesTable<ADSBRecentEntries>::draw(
	const Entry& entry,
	const Rect& target_rect,
	Painter& painter,
	const Style& style
) {
	painter.draw_string(
		target_rect.location(),
		style,
		to_string_hex(entry.ICAO_address, 6) + " " + entry.callsign + " " + (entry.hits <= 9999 ? to_string_dec_uint(entry.hits, 5) : "9999+") + " " + entry.time
		//to_string_hex_array((uint8_t*)entry.raw_data, 10) + " " + entry.time
	);
}

void ADSBRxView::focus() {
	field_lna.focus();
}

ADSBRxView::~ADSBRxView() {
	receiver_model.disable();
	baseband::shutdown();
}

void ADSBRxView::on_frame(const ADSBFrameMessage * message) {
	rtc::RTC datetime;
	std::string str_timestamp;
	std::string callsign;
	
	auto frame = message->frame;
	
	uint32_t ICAO_address = frame.get_ICAO_address();
	
	
	if (frame.check_CRC() && frame.get_ICAO_address()) {
		auto& entry = ::on_packet(recent, ICAO_address);
		
		rtcGetTime(&RTCD1, &datetime);
		str_timestamp = to_string_dec_uint(datetime.hour(), 2, '0') + ":" +
						to_string_dec_uint(datetime.minute(), 2, '0') + ":" +
						to_string_dec_uint(datetime.second(), 2, '0');
		
		entry.set_time(str_timestamp);
		entry.set_raw(frame.get_raw_data());
		entry.inc_hit();
		
		if (frame.get_DF() == DF_ADSB) {
			if (frame.get_msg_type() == TC_IDENT) {
				callsign = decode_frame_id(frame);
				entry.set_callsign(callsign);
			} else if (frame.get_msg_type() == TC_AIRBORNE_POS) {
				callsign = "Altitude: " + to_string_dec_uint(decode_frame_pos(frame)) + "ft";
				entry.set_pos(callsign);
			}
		}
		
		recent_entries_view.set_dirty();
	}
}

ADSBRxView::ADSBRxView(NavigationView& nav) {
	baseband::run_image(portapack::spi_flash::image_tag_adsb_rx);

	add_children({
		&labels,
		&rssi,
		&field_lna,
		&field_vga,
		&text_debug_a,
		&text_debug_b,
		&text_debug_c,
		&recent_entries_view
	});
	
	recent_entries_view.set_parent_rect({ 0, 64, 240, 224 });
	recent_entries_view.on_select = [this](const ADSBRecentEntry& entry) {
		text_debug_a.set(entry.geo_pos);
	};
	
	baseband::set_adsb();
	
	receiver_model.set_tuning_frequency(1090000000);
	receiver_model.set_rf_amp(true);
	field_lna.set_value(40);
	field_vga.set_value(40);
	receiver_model.set_modulation(ReceiverModel::Mode::SpectrumAnalysis);
	receiver_model.set_sampling_rate(2000000);
	receiver_model.set_baseband_bandwidth(2500000);
	receiver_model.enable();
}

} /* namespace ui */
