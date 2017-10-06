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

//#include "manchester.hpp"

#include "string_format.hpp"

/*void SondeLogger::on_packet(const sonde::Packet& packet) {
	const auto formatted = packet.symbols_formatted();
	log_file.write_entry(packet.received_at(), formatted.data + "/" + formatted.errors);
}*/

namespace ui {

SondeView::SondeView(NavigationView& nav) {
	baseband::run_image(portapack::spi_flash::image_tag_sonde);

	add_children({
		&field_frequency,
		&text_debug,
		&field_rf_amp,
		&field_lna,
		&field_vga,
		&rssi
	});

	field_frequency.set_value(receiver_model.tuning_frequency());
	field_frequency.set_step(receiver_model.frequency_step());
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
	
	radio::enable({
		receiver_model.tuning_frequency(),
		sampling_rate,
		baseband_bandwidth,
		rf::Direction::Receive,
		receiver_model.rf_amp(),
		static_cast<int8_t>(receiver_model.lna()),
		static_cast<int8_t>(receiver_model.vga()),
	});
	
	set_target_frequency(402000000);

	/*logger = std::make_unique<SondeLogger>();
	if( logger ) {
		logger->append(u"sonde.txt");
	}*/
}

SondeView::~SondeView() {
	radio::disable();
	baseband::shutdown();
}

void SondeView::focus() {
	field_vga.focus();
}

void SondeView::on_packet(const baseband::Packet& packet) {
	std::string bin_string;
	
	for (size_t i = 0; i < 30; i++) {
		bin_string += to_string_dec_uint(packet[i]);
	}
	
	text_debug.set(bin_string);
	
	/*if( logger ) {
		logger->on_packet(packet);
	}*/

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
