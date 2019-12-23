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

#include "rtc_time.hpp"
#include "string_format.hpp"
#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;

namespace ui {

template<>
void RecentEntriesTable<AircraftRecentEntries>::draw(
	const Entry& entry,
	const Rect& target_rect,
	Painter& painter,
	const Style& style
) {
	char aged_color;
	Color target_color;
	auto entry_age = entry.age;
	
	// Color decay for flights not being updated anymore
	if (entry_age < ADSB_DECAY_A) {
		aged_color = 0x10;
		target_color = Color::green();
	} else if ((entry_age >= ADSB_DECAY_A) && (entry_age < ADSB_DECAY_B)) {
		aged_color = 0x07;
		target_color = Color::light_grey();
	} else {
		aged_color = 0x08;
		target_color = Color::dark_grey();
	}
	
	std::string entry_string = "\x1B";
	entry_string += aged_color;
	entry_string += to_string_hex(entry.ICAO_address, 6) + " " +
		entry.callsign + "  " +
		(entry.hits <= 999 ? to_string_dec_uint(entry.hits, 4) : "999+") + " " + 
		entry.time_string;
	
	painter.draw_string(
		target_rect.location(),
		style,
		entry_string
	);
	
	if (entry.pos.valid)
		painter.draw_bitmap(target_rect.location() + Point(15 * 8, 0), bitmap_target, target_color, style.background);
}

void ADSBLogger::log_str(std::string& logline) {
	rtc::RTC datetime;
	rtcGetTime(&RTCD1, &datetime);
	log_file.write_entry(datetime,logline);
}

void ADSBRxDetailsView::focus() {
	button_see_map.focus();
}

void ADSBRxDetailsView::update(const AircraftRecentEntry& entry) {
	entry_copy = entry;
	uint32_t age = entry_copy.age;
	
	if (age < 60)
		text_last_seen.set(to_string_dec_uint(age) + " seconds ago");
	else
		text_last_seen.set(to_string_dec_uint(age / 60) + " minutes ago");
	
	text_infos.set(entry_copy.info_string);
	
	text_frame_pos_even.set(to_string_hex_array(entry_copy.frame_pos_even.get_raw_data(), 14));
	text_frame_pos_odd.set(to_string_hex_array(entry_copy.frame_pos_odd.get_raw_data(), 14));
	
	if (send_updates)
		geomap_view->update_position(entry_copy.pos.latitude, entry_copy.pos.longitude);
}

ADSBRxDetailsView::~ADSBRxDetailsView() {
	on_close_();
}

ADSBRxDetailsView::ADSBRxDetailsView(
	NavigationView& nav,
	const AircraftRecentEntry& entry,
	const std::function<void(void)> on_close
) : entry_copy(entry),
	on_close_(on_close)
{
	char file_buffer[32] { 0 };
	bool found = false;
	std::string airline_code;
	size_t c;
	
	add_children({
		&labels,
		&text_callsign,
		&text_last_seen,
		&text_airline,
		&text_country,
		&text_infos,
		&text_frame_pos_even,
		&text_frame_pos_odd,
		&button_see_map
	});
	
	std::unique_ptr<ADSBLogger> logger { };
	update(entry_copy);

	// The following won't (shouldn't !) change for a given airborne aircraft
	// Try getting the airline's name from airlines.db
	auto result = db_file.open("ADSB/airlines.db");
	if (!result.is_valid()) {
		// Search for 3-letter code in 0x0000~0x2000
		airline_code = entry_copy.callsign.substr(0, 3);
		c = 0;
		do {
			db_file.read(file_buffer, 4);
			if (!file_buffer[0])
				break;
			if (!airline_code.compare(0, 4, file_buffer))
				found = true;
			else
				c++;
		} while (!found);
		
		if (found) {
			db_file.seek(0x2000 + (c << 6));
			db_file.read(file_buffer, 32);
			text_airline.set(file_buffer);
			db_file.read(file_buffer, 32);
			text_country.set(file_buffer);
		} else {
			text_airline.set("Unknown");
			text_country.set("Unknown");
		}
	} else {
		text_airline.set("No airlines.db file");
		text_country.set("No airlines.db file");
	}
	
	text_callsign.set(entry_copy.callsign);
	
	button_see_map.on_select = [this, &nav](Button&) {
		geomap_view = nav.push<GeoMapView>(
			entry_copy.callsign,
			entry_copy.pos.altitude,
			GeoPos::alt_unit::FEET,
			entry_copy.pos.latitude,
			entry_copy.pos.longitude,
			0,
			[this]() {
				send_updates = false;
			});
		send_updates = true;
	};
};

void ADSBRxView::focus() {
	field_vga.focus();
}

ADSBRxView::~ADSBRxView() {
	rtc_time::signal_tick_second -= signal_token_tick_second;
	receiver_model.disable();
	baseband::shutdown();
}

void ADSBRxView::on_frame(const ADSBFrameMessage * message) {
	rtc::RTC datetime;
	std::string str_timestamp;
	std::string callsign;
	std::string str_info;
	std::string logentry;

	auto frame = message->frame;
	uint32_t ICAO_address = frame.get_ICAO_address();
	
	if (frame.check_CRC() && frame.get_ICAO_address()) {
		rtcGetTime(&RTCD1, &datetime);
		auto& entry = ::on_packet(recent, ICAO_address);
		frame.set_rx_timestamp(datetime.minute() * 60 + datetime.second());
		entry.reset_age();
		str_timestamp = to_string_datetime(datetime, HMS);
		entry.set_time_string(str_timestamp);

		entry.inc_hit();
		logentry += to_string_hex_array(frame.get_raw_data(), 14) + " ";
		logentry += "ICAO:" + to_string_hex(ICAO_address, 6) + " ";
		
		if (frame.get_DF() == DF_ADSB) {
			uint8_t msg_type = frame.get_msg_type();
			uint8_t * raw_data = frame.get_raw_data();
			
			if ((msg_type >= 1) && (msg_type <= 4)) {
				callsign = decode_frame_id(frame);
				entry.set_callsign(callsign);
				logentry+=callsign+" ";
			} else if (((msg_type >= 9) && (msg_type <= 18)) || ((msg_type >= 20) && (msg_type <= 22))) {
				entry.set_frame_pos(frame, raw_data[6] & 4);
				
				if (entry.pos.valid) {
					str_info = "Alt:" + to_string_dec_uint(entry.pos.altitude) +
						" Lat" + to_string_dec_int(entry.pos.latitude) +
						"." + to_string_dec_int((int)abs(entry.pos.latitude * 1000) % 100, 2, '0') +
						" Lon" + to_string_dec_int(entry.pos.longitude) +
						"." + to_string_dec_int((int)abs(entry.pos.longitude * 1000) % 100, 2, '0');
					
					entry.set_info_string(str_info);
					logentry+=str_info+ " ";

					if (send_updates)
						details_view->update(entry);
				}
			}
		}
		recent_entries_view.set_dirty(); 
		
		logger = std::make_unique<ADSBLogger>();
        if (logger) {
                logger->append(u"adsb.txt");
                // will log each frame in format:
                // 20171103100227 8DADBEEFDEADBEEFDEADBEEFDEADBEEF ICAO:nnnnnn callsign Alt:nnnnnn Latnnn.nn Lonnnn.nn
				logger->log_str(logentry);
        }
	}
}

void ADSBRxView::on_tick_second() {
	// Decay and refresh if needed
	for (auto& entry : recent) {
		entry.inc_age();
		
		if (details_view) {
			if (send_updates && (entry.key() == detailed_entry_key))
				details_view->update(entry);
		} else {
			if ((entry.age == ADSB_DECAY_A) || (entry.age == ADSB_DECAY_B))
				recent_entries_view.set_dirty();
		}
	}
}

ADSBRxView::ADSBRxView(NavigationView& nav) {
	baseband::run_image(portapack::spi_flash::image_tag_adsb_rx);
	add_children({
		&labels,
		&field_lna,
		&field_vga,
		&field_rf_amp,
		&rssi,
		&recent_entries_view
	});
	
	recent_entries_view.set_parent_rect({ 0, 16, 240, 272 });
	recent_entries_view.on_select = [this, &nav](const AircraftRecentEntry& entry) {
		detailed_entry_key = entry.key();
		details_view = nav.push<ADSBRxDetailsView>(
			entry,
			[this]() {
				send_updates = false;
			});
		send_updates = true;
	};
	
	signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
		on_tick_second();
	};
	
	baseband::set_adsb();
	
	receiver_model.set_tuning_frequency(1090000000);
	field_lna.set_value(40);
	field_vga.set_value(40);
	receiver_model.set_modulation(ReceiverModel::Mode::SpectrumAnalysis);
	receiver_model.set_sampling_rate(2000000);
	receiver_model.set_baseband_bandwidth(2500000);
	receiver_model.enable();
}

} /* namespace ui */
