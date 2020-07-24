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

#include "ui_sonde.hpp"
#include "baseband_api.hpp"

#include "portapack.hpp"
using namespace portapack;

#include "string_format.hpp"

void SondeLogger::on_packet(const sonde::Packet& packet) {
	const auto formatted = packet.symbols_formatted();
	log_file.write_entry(packet.received_at(), formatted.data);
}

namespace ui {

SondeView::SondeView(NavigationView& nav) {
	baseband::run_image(portapack::spi_flash::image_tag_sonde);

	add_children({
		&labels,
		&field_frequency,
		&field_rf_amp,
		&field_lna,
		&field_vga,
		&rssi,
		&check_log,
		&text_signature,
		&text_serial,
		&text_voltage,
		&geopos,
		&button_see_map
	});

	field_frequency.set_value(target_frequency_);
	field_frequency.set_step(10000);
	field_frequency.on_change = [this](rf::Frequency f) {
		set_target_frequency(f);
		field_frequency.set_value(f);
	};
	field_frequency.on_edit = [this, &nav]() {
		// TODO: Provide separate modal method/scheme?
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			set_target_frequency(f);
			field_frequency.set_value(f);
		};
	};
	
	geopos.set_read_only(true);
	
	check_log.on_select = [this](Checkbox&, bool v) {
		logging = v;
	};
	
	radio::enable({
		tuning_frequency(),
		sampling_rate,
		baseband_bandwidth,
		rf::Direction::Receive,
		receiver_model.rf_amp(),
		static_cast<int8_t>(receiver_model.lna()),
		static_cast<int8_t>(receiver_model.vga()),
	});

	button_see_map.on_select = [this, &nav](Button&) {
		nav.push<GeoMapView>(
			"",
			altitude,
			GeoPos::alt_unit::METERS,
			latitude,
			longitude,
			999); //set a dummy heading out of range to draw a cross...probably not ideal?
	};
	
	logger = std::make_unique<SondeLogger>();
	if (logger)
		logger->append(u"sonde.txt");
	
}

SondeView::~SondeView() {
	radio::disable();
	baseband::shutdown();
}

void SondeView::focus() {
	field_vga.focus();
}

void SondeView::on_packet(const sonde::Packet& packet) {
	//const auto hex_formatted = packet.symbols_formatted();
	
	text_signature.set(packet.type_string());
	text_serial.set(packet.serial_number());
	text_voltage.set(unit_auto_scale(packet.battery_voltage(), 2, 3) + "V");
	
	altitude = packet.GPS_altitude();
	latitude = packet.GPS_latitude();
	longitude = packet.GPS_longitude();
	
	geopos.set_altitude(altitude);
	geopos.set_lat(latitude);
	geopos.set_lon(longitude);
	
	if (logger && logging) {
		logger->on_packet(packet);
	}
	
	/*if( packet.crc_ok() ) {
	}*/
}

void SondeView::set_target_frequency(const uint32_t new_value) {
	target_frequency_ = new_value;
	radio::set_tuning_frequency(tuning_frequency());
}

uint32_t SondeView::tuning_frequency() const {
	return target_frequency_ - (sampling_rate / 4);
}

} /* namespace ui */
