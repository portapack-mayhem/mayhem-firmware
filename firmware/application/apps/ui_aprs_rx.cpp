/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui_aprs_rx.hpp"

#include "audio.hpp"
#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;

void APRSLogger::log_raw_data(const std::string& data) {
	rtc::RTC datetime;
	rtcGetTime(&RTCD1, &datetime);
	
	log_file.write_entry(datetime, data);
}

namespace ui {

template<>
void RecentEntriesTable<APRSRecentEntries>::draw(
	const Entry& entry,
	const Rect& target_rect,
	Painter& painter,
	const Style& style
) {
	char aged_color;
	Color target_color;
	// auto entry_age = entry.age;
	
	target_color = Color::green();

	aged_color = 0x10;
	std::string entry_string = "\x1B";
	entry_string += aged_color;
	
	entry_string += entry.source_formatted;
	entry_string.append(10 - entry.source_formatted.size(),' ');
	entry_string += "       ";
	entry_string += (entry.hits <= 999 ? to_string_dec_uint(entry.hits, 4) : "999+");
	entry_string += " ";
	entry_string += entry.time_string;
	
	painter.draw_string(
		target_rect.location(),
		style,
		entry_string
	);
	
	if (entry.has_position){
		painter.draw_bitmap(target_rect.location() + Point(12 * 8, 0), bitmap_target, target_color, style.background);
	}
}


void APRSRxView::focus() {
	options_region.focus();
}

void APRSRxView::update_freq(rf::Frequency f) {
	receiver_model.set_tuning_frequency(f);
}

APRSRxView::APRSRxView(NavigationView& nav, Rect parent_rect) : View(parent_rect) {
	baseband::run_image(portapack::spi_flash::image_tag_aprs_rx);
	
	add_children({
		&rssi,
		&channel,
		&field_rf_amp,
		&field_lna,
		&field_vga,
		&options_region,
		&field_frequency,
		&record_view,
		&console
	});
	
	// load app settings
	auto rc = settings.load("rx_aprs", &app_settings);
	if(rc == SETTINGS_OK) {
		field_lna.set_value(app_settings.lna);
		field_vga.set_value(app_settings.vga);
		field_rf_amp.set_value(app_settings.rx_amp);
	}


	// DEBUG
	record_view.on_error = [&nav](std::string message) {
		nav.display_modal("Error", message);
	};

	record_view.set_sampling_rate(24000);

	options_region.on_change = [this](size_t, int32_t i) {		
		if (i == 0){
			field_frequency.set_value(144390000);			
		} else if(i == 1){
			field_frequency.set_value(144800000);
		} else if(i == 2){
			field_frequency.set_value(145175000);
		} else if(i == 3){
			field_frequency.set_value(144575000);
		}		
	};
	
	field_frequency.set_value(receiver_model.tuning_frequency());
	field_frequency.set_step(100);
	field_frequency.on_change = [this](rf::Frequency f) {
		update_freq(f);
	};
	field_frequency.on_edit = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			update_freq(f);
			field_frequency.set_value(f);
		};
	};
	
	options_region.set_selected_index(0, true);
	
	logger = std::make_unique<APRSLogger>();
	if (logger)
		logger->append("APRS_RX_LOG.TXT");
	
	baseband::set_aprs(1200);
	
	audio::set_rate(audio::Rate::Hz_24000);
	audio::output::start();
	
	receiver_model.set_sampling_rate(3072000);
	receiver_model.set_baseband_bandwidth(1750000);
	receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio);
	receiver_model.enable();
}

void APRSRxView::on_packet(const APRSPacketMessage* message){
	std::string str_console = "\x1B";

	aprs::APRSPacket packet = message->packet;

	std::string stream_text = packet.get_stream_text();
	str_console += (char)((console_color++ & 3) + 9);		
	str_console += stream_text;

	if(logger){
		logger->log_raw_data(stream_text);
	}
	
	//if(reset_console){ //having more than one console causes issues when switching tabs where one is disabled, and the other enabled breaking the scoll setup.
	//	console.on_hide();
	//	console.on_show();
	//	reset_console = false;
	//}

	console.writeln(str_console);
}

void APRSRxView::on_data(uint32_t value, bool is_data) {
	std::string str_console = "\x1B";
	std::string str_byte = "";
	
	if (is_data) {
		// Colorize differently after message splits
		str_console += (char)((console_color & 3) + 9);		

		if (value == '\n') {
			// Message split
			console.writeln("");
			console_color++;
			
			if (logger) {
				logger->log_raw_data(str_log);
				str_log = "";
			}
		}
		else {
			if ((value >= 32) && (value < 127)) {
				str_console += (char)value;							// Printable
				str_byte += (char)value;
			} else {
				str_console += "[" + to_string_hex(value, 2) + "]";	// Not printable
				str_byte += "[" + to_string_hex(value, 2) + "]";
			}

			console.write(str_console);
		
			if (logger) str_log += str_byte;
		}
	} else {

	}
}

void APRSRxView::on_show(){
	//some bug where the display scroll area is set to the entire screen when switching from the list tab with details showing back to the stream view.
	//reset_console = true;	
}

APRSRxView::~APRSRxView() {

	// save app settings
	settings.save("rx_aprs", &app_settings);

	audio::output::stop();
	receiver_model.disable();
	baseband::shutdown();
}

void APRSTableView::on_show_list() {
	details_view.hidden(true);
	recent_entries_view.hidden(false);	
	send_updates = false;
	focus();
}

void APRSTableView::on_show_detail(const APRSRecentEntry& entry) {
	recent_entries_view.hidden(true);
	details_view.hidden(false);
	details_view.set_entry(entry);
	details_view.update();
	details_view.focus();
	detailed_entry_key = entry.key();
	send_updates = true;
}

APRSTableView::APRSTableView(NavigationView& nav, Rect parent_rec) : View(parent_rec),  nav_ {nav}  {
	add_children({		
		&recent_entries_view,
		&details_view
	});

	hidden(true);

	details_view.hidden(true);
	
	recent_entries_view.set_parent_rect({0, 0, 240, 280});
	details_view.set_parent_rect({0, 0, 240, 280});
	
	recent_entries_view.on_select = [this](const APRSRecentEntry& entry) {
		this->on_show_detail(entry);
	};

	details_view.on_close = [this]() {
		this->on_show_list();
	};


/*	for(size_t i = 0; i <32 ; i++){
		std::string id = "test" + i;
		auto& entry = ::on_packet(recent, i);			
		entry.set_source_formatted(id);	
	}

	recent_entries_view.set_dirty(); 	*/

	/*

	std::string str1 = "test1";
	std::string str12 = "test2";
	std::string str13 = "test2";

	auto& entry = ::on_packet(recent, 0x1);			
	entry.set_source_formatted(str1);	

	auto& entry2 = ::on_packet(recent, 0x2);			
	entry2.set_source_formatted(str12);	

	auto& entry3 = ::on_packet(recent, 0x2);			
	entry2.set_source_formatted(str13);	

	recent_entries_view.set_dirty(); 	

	*/
}

void APRSTableView::on_pkt(const APRSPacketMessage* message){
	aprs::APRSPacket packet = message->packet;
	rtc::RTC datetime;	
	std::string str_timestamp;
	std::string source_formatted = packet.get_source_formatted();
	std::string info_string = packet.get_stream_text();

	rtcGetTime(&RTCD1, &datetime);
	auto& entry = ::on_packet(recent, packet.get_source());		
	entry.reset_age();
	entry.inc_hit();
	str_timestamp = to_string_datetime(datetime, HMS);
	entry.set_time_string(str_timestamp);
	entry.set_info_string(info_string);

	entry.set_source_formatted(source_formatted);

	if(entry.has_position && !packet.has_position()){ 
		//maintain position info
	}
	else {
		entry.set_has_position(packet.has_position());
		entry.set_pos(packet.get_position());
	}	

	if( entry.key() == details_view.entry().key() ) {
		details_view.set_entry(entry);
		details_view.update();		
	}

	recent_entries_view.set_dirty(); 			
}

void APRSTableView::on_show(){
	on_show_list();
}

void APRSTableView::on_hide(){
	details_view.hidden(true);
}

void APRSTableView::focus(){
	recent_entries_view.focus();
}

APRSTableView::~APRSTableView(){

}

void APRSDetailsView::focus() {
	button_done.focus();
}

void APRSDetailsView::set_entry(const APRSRecentEntry& entry){
	entry_copy = entry;
}

void APRSDetailsView::update() {	
	if(!hidden()){		
		//uint32_t age = entry_copy.age;
		
		console.clear(true);
		console.write(entry_copy.info_string);

		button_see_map.hidden(!entry_copy.has_position);
	}	
	
	if (send_updates)
		geomap_view->update_position(entry_copy.pos.latitude, entry_copy.pos.longitude, 0, 0);
}

APRSDetailsView::~APRSDetailsView() {
}

APRSDetailsView::APRSDetailsView(
	NavigationView& nav	
) 
{	
	add_children({
		&console,
		&button_done,
		&button_see_map
	});

	button_done.on_select = [this, &nav](Button&) {
		console.clear(true);
		this->on_close();
	};

	button_see_map.on_select = [this, &nav](Button&) {
		geomap_view = nav.push<GeoMapView>(
			entry_copy.source_formatted,			
			0, //entry_copy.pos.altitude,
			GeoPos::alt_unit::FEET,
			entry_copy.pos.latitude,
			entry_copy.pos.longitude,			
			0, /*entry_copy.velo.heading,*/
			[this]() {
				send_updates = false;
				hidden(false);
				update();
			});		
		send_updates = true;
		hidden(true);
		
	};
};

APRSRXView::APRSRXView(NavigationView& nav) : nav_ {nav} {
	add_children({
		&tab_view,
		&view_stream,
		&view_table
	});
}

void APRSRXView::focus(){
	tab_view.focus();
}

APRSRXView::~APRSRXView() {
	
}
} /* namespace ui */
