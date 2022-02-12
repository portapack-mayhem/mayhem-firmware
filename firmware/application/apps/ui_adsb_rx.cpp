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

#include <strings.h>

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
	} else if (entry_age < ADSB_DECAY_B) {
		aged_color = 0x07;
		target_color = Color::light_grey();
	} else {
		aged_color = 0x08;
		target_color = Color::dark_grey();
	}
	
	std::string entry_string = "\x1B";
	entry_string += aged_color;
#if false
	entry_string += to_string_hex(entry.ICAO_address, 6) + " " +
		entry.callsign + "  " +
		(entry.hits <= 999 ? to_string_dec_uint(entry.hits, 4) : "999+") + " " + 
		entry.time_string;
#else
	// SBT
	entry_string += 
		(entry.callsign[0]!=' ' ? entry.callsign + " " : to_string_hex(entry.ICAO_address, 6) + "   ") +
		to_string_dec_uint((unsigned int)((entry.pos.altitude+50)/100),4) +
		to_string_dec_uint((unsigned int)entry.velo.speed,4) +
		to_string_dec_uint((unsigned int)(entry.amp>>9),4) + " " +
		(entry.hits <= 999 ? to_string_dec_uint(entry.hits, 3) + " " : "1k+ ") +
		to_string_dec_uint(entry.age, 4);
#endif
	
	painter.draw_string(
		target_rect.location(),
		style,
		entry_string
	);
	
	if (entry.pos.valid)
		painter.draw_bitmap(target_rect.location() + Point(8 * 8, 0), bitmap_target, target_color, style.background);
}

void ADSBLogger::log_str(std::string& logline) {
	rtc::RTC datetime;
	rtcGetTime(&RTCD1, &datetime);
	log_file.write_entry(datetime,logline);
}


// Aircraft Details
void ADSBRxAircraftDetailsView::focus() {
	button_close.focus();
}

ADSBRxAircraftDetailsView::~ADSBRxAircraftDetailsView() {
	on_close_();
}

ADSBRxAircraftDetailsView::ADSBRxAircraftDetailsView(
	NavigationView& nav,
	const AircraftRecentEntry& entry,
	const std::function<void(void)> on_close
) : entry_copy(entry),
	on_close_(on_close)
{
	char file_buffer[32] { 0 };
	bool found = false;
	size_t number_of_icao_codes = 0;	
	std::string icao_code;
	
	add_children({
		&labels,
		&text_icao_address,
		&text_registration,
		&text_manufacturer,
		&text_model,
		&text_type,
		&text_number_of_engines,
		&text_engine_type,
		&text_owner,
		&text_operator,
		&button_close
	});
	
	std::unique_ptr<ADSBLogger> logger { };
	//update(entry_copy);

	text_icao_address.set(to_string_hex(entry_copy.ICAO_address, 6));


	// Try getting the aircraft information from icao24.db
	auto result = db_file.open("ADSB/icao24.db");
	if (!result.is_valid()) {
		// determine number of ICAO24 codes in file, total size / (single index + record size)
		number_of_icao_codes = (db_file.size() / 153); 
		icao_code = to_string_hex(entry_copy.ICAO_address, 6);

  		// binary search
    		int first = 0,         				// First search element       
    		last = number_of_icao_codes - 1,        	// Last search element       
    		middle,                				// Mid point of search       
    		position = -1;         				// Position of search value   
    		while (!found && first <= last) {  
        		middle = (first + last) / 2;     	// Calculate mid point      
        		db_file.seek(middle * 7); 
			db_file.read(file_buffer, 6);
			if (file_buffer == icao_code) {     	// If value is found at mid        
                		found = true;         
                		position = middle;      
        		}      
        		else if (file_buffer > icao_code)  	// If value is in lower half         
            			last = middle - 1;      
        		else         
            			first = middle + 1;          	// If value is in upper half   
    		}   
		
		if (position > -1) {
			db_file.seek((number_of_icao_codes * 7) + (position * 146)); // seek starting after index
			db_file.read(file_buffer, 9);
			text_registration.set(file_buffer);
			db_file.read(file_buffer, 33);
			text_manufacturer.set(file_buffer);
			db_file.read(file_buffer, 33);
			text_model.set(file_buffer);
			db_file.read(file_buffer, 5); // ICAO type decripton
			if(strlen(file_buffer) == 3) {
				switch(file_buffer[0]) {
					case 'L': 
						text_type.set("Landplane"); 
						break;
					case 'S': 
						text_type.set("Seaplane"); 
						break;
					case 'A': 
						text_type.set("Amphibian"); 
						break;
					case 'H': 
						text_type.set("Helicopter"); 
						break;
					case 'G': 
						text_type.set("Gyrocopter"); 
						break;
					case 'T': 
						text_type.set("Tilt-wing aircraft"); 
						break;					
				}
				text_number_of_engines.set(std::string(1, file_buffer[1]));
				switch(file_buffer[2]) {
					case 'P': 
						text_engine_type.set("Piston engine"); 
						break;
					case 'T': 
						text_engine_type.set("Turboprop/Turboshaft engine"); 
						break;
					case 'J': 
						text_engine_type.set("Jet engine"); 
						break;
					case 'E': 
						text_engine_type.set("Electric engine"); 
						break;
				}

			}
			// check for ICAO type designator
			else if(strlen(file_buffer) == 4) {
				if(strcmp(file_buffer,"SHIP") == 0) text_type.set("Airship");
				else if(strcmp(file_buffer,"BALL") == 0) text_type.set("Balloon");
				else if(strcmp(file_buffer,"GLID") == 0) text_type.set("Glider / sailplane");
				else if(strcmp(file_buffer,"ULAC") == 0) text_type.set("Micro/ultralight aircraft");
				else if(strcmp(file_buffer,"GYRO") == 0) text_type.set("Micro/ultralight autogyro");
				else if(strcmp(file_buffer,"UHEL") == 0) text_type.set("Micro/ultralight helicopter");
				else if(strcmp(file_buffer,"SHIP") == 0) text_type.set("Airship");
				else if(strcmp(file_buffer,"PARA") == 0) text_type.set("Powered parachute/paraplane");
			}			
			db_file.read(file_buffer, 33);
			text_owner.set(file_buffer);
			db_file.read(file_buffer, 33);
			text_operator.set(file_buffer);
		} else {
			text_registration.set("Unknown");
			text_manufacturer.set("Unknown");
		}
	} else {
		text_manufacturer.set("No icao24.db file");
	}


	button_close.on_select =  [&nav](Button&){
		nav.pop();
	};

};

// End of Aicraft details

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
	if(entry_copy.velo.heading < 360 && entry_copy.velo.speed >=0){ //I don't like this but...
		text_info2.set("Hdg:" + to_string_dec_uint(entry_copy.velo.heading) + " Spd:" + to_string_dec_int(entry_copy.velo.speed));
	}else{
		text_info2.set("");
	}
	text_frame_pos_even.set(to_string_hex_array(entry_copy.frame_pos_even.get_raw_data(), 14));
	text_frame_pos_odd.set(to_string_hex_array(entry_copy.frame_pos_odd.get_raw_data(), 14));
	
	if (send_updates)
		geomap_view->update_position(entry_copy.pos.latitude, entry_copy.pos.longitude, entry_copy.velo.heading, entry_copy.pos.altitude);
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
	size_t number_of_airlines = 0;	
	std::string airline_code;
	
	add_children({
		&labels,
		&text_icao_address,
		&text_callsign,
		&text_last_seen,
		&text_airline,
		&text_country,
		&text_infos,
		&text_info2,
		&text_frame_pos_even,
		&text_frame_pos_odd,
		&button_aircraft_details,
		&button_see_map
	});
	
	std::unique_ptr<ADSBLogger> logger { };
	update(entry_copy);

	// The following won't (shouldn't !) change for a given airborne aircraft
	// Try getting the airline's name from airlines.db
	auto result = db_file.open("ADSB/airlines.db");
	if (!result.is_valid()) {
		// Search for 3-letter code
		number_of_airlines = (db_file.size() / 68); // determine number of airlines in file
		airline_code = entry_copy.callsign.substr(0, 3);

  		// binary search
    		int first = 0,         				// First search element       
    		last = number_of_airlines - 1,        	 	// Last search element       
    		middle,                				// Mid point of search       
    		position = -1;         				// Position of search value   
    		while (!found && first <= last) {  
        		middle = (first + last) / 2;     	// Calculate mid point      
        		db_file.seek(middle * 4); 
			db_file.read(file_buffer, 3);
			if (file_buffer == airline_code) {     	// If value is found at mid        
                		found = true;         
                		position = middle;      
        		}      
        		else if (file_buffer > airline_code)  	// If value is in lower half         
            			last = middle - 1;      
        		else         
            			first = middle + 1;          	// If value is in upper half   
    		}   
		
		if (position > -1) {
			db_file.seek((number_of_airlines * 4) + (position << 6)); // seek starting after index
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
	text_icao_address.set(to_string_hex(entry_copy.ICAO_address, 6));

        button_aircraft_details.on_select = [this, &nav](Button&) {
		//detailed_entry_key = entry.key();
		aircraft_details_view = nav.push<ADSBRxAircraftDetailsView>(
			entry_copy,
			[this]() {
				send_updates = false;
			});
		send_updates = false;
	};

	button_see_map.on_select = [this, &nav](Button&) {
		if (!send_updates) { // Prevent recursively launching the map
			geomap_view = nav.push<GeoMapView>(
				entry_copy.callsign,
				entry_copy.pos.altitude,
				GeoPos::alt_unit::FEET,
				entry_copy.pos.latitude,
				entry_copy.pos.longitude,
				entry_copy.velo.heading,
				[this]() {
				send_updates = false;
			});
			send_updates = true;
		}
	};
};


void ADSBRxView::focus() {
	field_vga.focus();
}

ADSBRxView::~ADSBRxView() {
	receiver_model.set_tuning_frequency(prevFreq);

	rtc_time::signal_tick_second -= signal_token_tick_second;
	receiver_model.disable();
	baseband::shutdown();
}

AircraftRecentEntry ADSBRxView::find_or_create_entry(uint32_t ICAO_address) {
	auto it = find(recent, ICAO_address);

	// If not found
	if (it == std::end(recent)){
		recent.emplace_front(ICAO_address); // Add it
		truncate_entries(recent); // Truncate the list
		sort_entries_by_state();
		it = find(recent, ICAO_address); // Find it again
	}
	return *it;
}

void ADSBRxView::replace_entry(AircraftRecentEntry & entry)
{
	uint32_t ICAO_address = entry.ICAO_address;

	std::replace_if( recent.begin(), recent.end(), 
		[ICAO_address](const AircraftRecentEntry & compEntry) {return ICAO_address == compEntry.ICAO_address;},
		entry);
}

void ADSBRxView::sort_entries_by_state()
{
	// Sorting List pn age_state using lambda function as comparator
	recent.sort([](const AircraftRecentEntry & left, const AircraftRecentEntry & right){return (left.age_state < right.age_state); });
}

void ADSBRxView::on_frame(const ADSBFrameMessage * message) {
	rtc::RTC datetime;
	std::string str_timestamp;
	std::string callsign;
	std::string str_info;
	std::string logentry;

	auto frame = message->frame;
	uint32_t ICAO_address = frame.get_ICAO_address();

	if (frame.check_CRC() && ICAO_address) {
		rtcGetTime(&RTCD1, &datetime);
		auto entry = find_or_create_entry(ICAO_address);
		frame.set_rx_timestamp(datetime.minute() * 60 + datetime.second());
		entry.reset_age();
		if (entry.hits==0)
		{ 
			entry.amp = message->amp;
		} else {
			entry.amp = ((entry.amp*15)+message->amp)>>4;
		}
		str_timestamp = to_string_datetime(datetime, HMS);
		entry.set_time_string(str_timestamp);

		entry.inc_hit();
		logentry += to_string_hex_array(frame.get_raw_data(), 14) + " ";
		logentry += "ICAO:" + to_string_hex(ICAO_address, 6) + " ";
		
		if (frame.get_DF() == DF_ADSB) {
			uint8_t msg_type = frame.get_msg_type();
			uint8_t msg_sub = frame.get_msg_sub();
			uint8_t * raw_data = frame.get_raw_data();
			
			// 4: // surveillance, altitude reply
			if ((msg_type >= AIRCRAFT_ID_L) && (msg_type <= AIRCRAFT_ID_H)) {
				callsign = decode_frame_id(frame);
				entry.set_callsign(callsign);
				logentry+=callsign+" ";
			} 
			// 9:
			// 18: { // Extended squitter/non-transponder
			// 21: // Comm-B, identity reply
			// 20: // Comm-B, altitude reply
			else if (((msg_type >= AIRBORNE_POS_BARO_L) && (msg_type <= AIRBORNE_POS_BARO_H)) || 
				((msg_type >= AIRBORNE_POS_GPS_L) && (msg_type <= AIRBORNE_POS_GPS_H))) {
				entry.set_frame_pos(frame, raw_data[6] & 4);
				
				if (entry.pos.valid) {
					str_info = "Alt:" + to_string_dec_int(entry.pos.altitude) +
						" Lat:" + to_string_decimal(entry.pos.latitude, 2) +
						" Lon:" + to_string_decimal(entry.pos.longitude, 2);

					// printing the coordinates in the log file with more
					// resolution, as we are not constrained by screen 
					// real estate there:

					std::string log_info = "Alt:" + to_string_dec_int(entry.pos.altitude) +
						" Lat:" + to_string_decimal(entry.pos.latitude, 7) +
						" Lon:" + to_string_decimal(entry.pos.longitude, 7);

					entry.set_info_string(str_info);
					logentry+=log_info + " ";

				}
			} else if(msg_type == AIRBORNE_VEL && msg_sub >= VEL_GND_SUBSONIC && msg_sub <= VEL_AIR_SUPERSONIC){
				entry.set_frame_velo(frame);
				logentry += "Type:" + to_string_dec_uint(msg_sub) +
							" Hdg:" + to_string_dec_uint(entry.velo.heading) +
							" Spd: "+ to_string_dec_int(entry.velo.speed);

			}
		}

		replace_entry(entry);
		
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
			if (send_updates && (entry.key() == detailed_entry_key)) // Check if the ICAO address match
				details_view->update(entry);
		}
	}

	// Sort the list if it is being displayed
	if (!send_updates) {
		sort_entries_by_state();
		recent_entries_view.set_dirty();
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
	
	prevFreq = receiver_model.tuning_frequency();

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
