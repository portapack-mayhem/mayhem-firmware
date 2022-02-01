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
#include "audio.hpp"

#include "portapack.hpp"
#include <cstring>
#include <stdio.h>

using namespace portapack;

#include "string_format.hpp"
#include "complex.hpp"


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
		&field_volume,
		&check_beep,
		&check_log,
		&check_crc,
		&text_signature,
		&text_serial,
		&text_timestamp,
		&text_voltage,
		&text_frame,
		&text_temp,
		&text_humid,
		&geopos,
                &button_see_qr,
		&button_see_map
	});

	// start from the frequency currently stored in the receiver_model:
	target_frequency_ = receiver_model.tuning_frequency();

	field_frequency.set_value(target_frequency_);
	field_frequency.set_step(500);		//euquiq: was 10000, but we are using this for fine-tunning
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
	
	check_beep.on_select = [this](Checkbox&, bool v) {
		beep = v;
	};

	check_log.on_select = [this](Checkbox&, bool v) {
		logging = v;
	};

	check_crc.on_select = [this](Checkbox&, bool v) {
		use_crc = v;
	};
	
    receiver_model.set_tuning_frequency(tuning_frequency());
    receiver_model.set_sampling_rate(sampling_rate);
    receiver_model.set_baseband_bandwidth(baseband_bandwidth);
    receiver_model.enable();   // Before using radio::enable(), but not updating Ant.DC-Bias.
	
	/* radio::enable({        // this can be removed, previous version, no DC-bias ant. control.
		tuning_frequency(),
		sampling_rate,
		baseband_bandwidth,
		rf::Direction::Receive,
		receiver_model.rf_amp(),
		static_cast<int8_t>(receiver_model.lna()),
		static_cast<int8_t>(receiver_model.vga()),
	}); */


        // QR code with geo URI
	button_see_qr.on_select = [this, &nav](Button&) {
		nav.push<QRCodeView>(geo_uri);
	};

	button_see_map.on_select = [this, &nav](Button&) {
		nav.push<GeoMapView>(
			sonde_id,
			gps_info.alt,
			GeoPos::alt_unit::METERS,
			gps_info.lat,
			gps_info.lon,
			999); //set a dummy heading out of range to draw a cross...probably not ideal?
	};

	logger = std::make_unique<SondeLogger>();
	if (logger)
		logger->append(u"sonde.txt");

	// initialize audio:
	
	field_volume.set_value((receiver_model.headphone_volume() - audio::headphone::volume_range().max).decibel() + 99);
	
	field_volume.on_change = [this](int32_t v) {
		this->on_headphone_volume_changed(v);
	};

	audio::output::start();
	audio::output::speaker_unmute();

	// inject a PitchRSSIConfigureMessage in order to arm 
	// the pitch rssi events that will be used by the 
	// processor:
	const PitchRSSIConfigureMessage message { true, 0 };

	shared_memory.application_queue.push(message);

	baseband::set_pitch_rssi(0, true);
}

SondeView::~SondeView() {
	baseband::set_pitch_rssi(0, false);
/* 	radio::disable(); */  
    receiver_model.disable();   // to switch off all, including DC bias.
	baseband::shutdown();
	audio::output::stop();
}

void SondeView::focus() {
	field_vga.focus();
}


// used to convert float to character pointer, since unfortunately function like
// sprintf and c_str aren't supported.
char * SondeView::float_to_char(float x, char *p) 
{

    	char *s = p + 9; // go to end of buffer
    	uint16_t decimals;  // variable to store the decimals
    	int units;  // variable to store the units (part to left of decimal place)
    	if (x < 0) { // take care of negative numbers
        	decimals = (int)(x * -100000) % 100000; // make 1000 for 3 decimals etc.
        	units = (int)(-1 * x);
    	} else { // positive numbers
        	decimals = (int)(x * 100000) % 100000;
        	units = (int)x;
    	}

	// TODO: more elegant solution (loop?)
    	*--s = (decimals % 10) + '0';
    	decimals /= 10; 
    	*--s = (decimals % 10) + '0';
    	decimals /= 10; 
    	*--s = (decimals % 10) + '0';
    	decimals /= 10; 
    	*--s = (decimals % 10) + '0';
    	decimals /= 10; 
    	*--s = (decimals % 10) + '0';
    	*--s = '.';

    	while (units > 0) {
        	*--s = (units % 10) + '0';
        	units /= 10;
    	}
    	if (x < 0) *--s = '-'; // unary minus sign for negative numbers
    	return s;
}

void SondeView::on_packet(const sonde::Packet &packet)
{
	if (!use_crc || packet.crc_ok()) //euquiq: Reject bad packet if crc is on
	{

		char buffer_lat[10] = {};
		char buffer_lon[10] = {};

		strcpy(geo_uri, "geo:");
		strcat(geo_uri, float_to_char(gps_info.lat, buffer_lat));
        	strcat(geo_uri, ",");
		strcat(geo_uri, float_to_char(gps_info.lon, buffer_lon));

		text_signature.set(packet.type_string());

		sonde_id = packet.serial_number(); //used also as tag on the geomap
		text_serial.set(sonde_id);

		text_timestamp.set(to_string_timestamp(packet.received_at()));

		text_voltage.set(unit_auto_scale(packet.battery_voltage(), 2, 2) + "V");

		text_frame.set(to_string_dec_uint(packet.frame(),0)); //euquiq: integrate frame #, temp & humid.
		
		temp_humid_info = packet.get_temp_humid();
		if (temp_humid_info.humid != 0)
		{
			double decimals = abs(get_decimals(temp_humid_info.humid, 10, true));
			//if (decimals < 0)
			//	decimals = -decimals;
			text_humid.set(to_string_dec_int((int)temp_humid_info.humid) + "." + to_string_dec_uint(decimals, 1) + "%");
		}

		if (temp_humid_info.temp != 0)
		{
			double decimals = abs(get_decimals(temp_humid_info.temp, 10, true));
			// if (decimals < 0)
			// 	decimals = -decimals;
			text_temp.set(to_string_dec_int((int)temp_humid_info.temp) + "." + to_string_dec_uint(decimals, 1) + "C");
		}

		gps_info = packet.get_GPS_data();

		geopos.set_altitude(gps_info.alt);
		geopos.set_lat(gps_info.lat);
		geopos.set_lon(gps_info.lon);

		if (logger && logging) {
			logger->on_packet(packet);
		}

		if(beep) {
			baseband::request_beep();
		}
	}
}

void SondeView::on_headphone_volume_changed(int32_t v) {
	const auto new_volume = volume_t::decibel(v - 99) + audio::headphone::volume_range().max;
	receiver_model.set_headphone_volume(new_volume);
}

void SondeView::set_target_frequency(const uint32_t new_value) {
	target_frequency_ = new_value;
	//radio::set_tuning_frequency(tuning_frequency());
	// we better remember the tuned frequency, by using this function instead:
	receiver_model.set_tuning_frequency(target_frequency_);
}

uint32_t SondeView::tuning_frequency() const {
	return target_frequency_ - (sampling_rate / 4);
}

} /* namespace ui */
