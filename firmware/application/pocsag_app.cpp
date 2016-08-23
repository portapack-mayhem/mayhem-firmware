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

#include "pocsag_app.hpp"

#include "baseband_api.hpp"

#include "portapack.hpp"
using namespace portapack;

#include "string_format.hpp"

#include "utility.hpp"

#define POCSAG_IDLE		0x7A89C197

namespace pocsag {

namespace format {

static std::string signal_rate_str(SignalRate signal_rate) {
	switch(signal_rate) {
		case SignalRate::FSK512:	return "FSK 512 ";
		case SignalRate::FSK1200:	return "FSK 1200";
		case SignalRate::FSK2400:	return "FSK 2400";
		default:					return "- - - - ";
	}
}

} /* namespace format */

} /* namespace pocsag */

void POCSAGLogger::on_packet(const pocsag::POCSAGPacket& packet) {
	std::string entry = pocsag::format::signal_rate_str(packet.signal_rate()) + " ";
	for (size_t c = 0; c < 16; c++)
		entry += to_string_hex(packet[c], 8) + " ";
	
	log_file.write_entry(packet.timestamp(), entry);
}

namespace ui {

POCSAGAppView::POCSAGAppView(NavigationView& nav) {
	baseband::run_image(portapack::spi_flash::image_tag_pocsag);

	add_children({ {
		&rssi,
		&channel,
		&options_band,
		&field_rf_amp,
		&field_lna,
		&field_vga,
		//&text_debug,
		&console
	} });

	radio::enable({
		tuning_frequency(),
		sampling_rate,
		baseband_bandwidth,
		rf::Direction::Receive,
		receiver_model.rf_amp(),
		static_cast<int8_t>(receiver_model.lna()),
		static_cast<int8_t>(receiver_model.vga()),
		1,
	});

	options_band.on_change = [this](size_t, OptionsField::value_t v) {
		this->on_band_changed(v);
	};
	options_band.set_by_value(target_frequency());

	logger = std::make_unique<POCSAGLogger>();
	if( logger ) {
		logger->append("pocsag.txt");
	}
	
	baseband::set_pocsag();
}

POCSAGAppView::~POCSAGAppView() {
	radio::disable();
	baseband::shutdown();
}

void POCSAGAppView::focus() {
	options_band.focus();
}

void POCSAGAppView::set_parent_rect(const Rect new_parent_rect) {
	View::set_parent_rect(new_parent_rect);
}

void POCSAGAppView::on_packet(const POCSAGPacketMessage * message) {
	bool eom = false;
	uint32_t codeword;
	
	if( logger ) {
		logger->on_packet(message->packet);
	}
	
	for (size_t c = 0; c < 16; c++) {
		codeword = message->packet[c];
		
		if (codeword & 0x80000000) {
			// Message
			console.writeln("Message !");
			
		} else {
			// Address
			if (codeword == POCSAG_IDLE) {
				eom = true;
			} else {
				function = (codeword >> 11) & 3;
				address  = ((codeword >> 10) & 0x1FFFF8) | ((codeword >> 1) & 7);
			}
		}
	}

	if (eom)
		console.writeln("Address:" + to_string_hex(address, 6) + " Function:" + to_string_dec_uint(function));
		//console.writeln("EOM");
	//console.writeln(to_string_hex(message->packet[0], 8) + "/"+ to_string_hex(message->packet[1], 8));
	
	if (eom)
		batch_cnt = 0;
	else
		batch_cnt++;
}

void POCSAGAppView::on_band_changed(const uint32_t new_band_frequency) {
	set_target_frequency(new_band_frequency);
}

void POCSAGAppView::set_target_frequency(const uint32_t new_value) {
	target_frequency_ = new_value;
	radio::set_tuning_frequency(tuning_frequency());
}

uint32_t POCSAGAppView::target_frequency() const {
	return target_frequency_;
}

uint32_t POCSAGAppView::tuning_frequency() const {
	return target_frequency() - (sampling_rate / 4);
}

} /* namespace ui */
