/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui_afskrx.hpp"

#include "ui_spectrum.hpp"
#include "ui_console.hpp"

#include "ff.h"

#include "portapack.hpp"
using namespace portapack;

#include "ui_receiver.hpp"

namespace ui {

AFSKRXView::AFSKRXView(
	NavigationView& nav,
	ReceiverModel& receiver_model
) : receiver_model(receiver_model)
{
	add_children({ {
		&button_done,
		&text_rx
	} }	);

	button_done.on_select = [&nav](Button&){
		nav.pop();
	};
	
	receiver_model.set_baseband_configuration({
		.mode = 6,
		.sampling_rate = 3072000,
		.decimation_factor = 4,
	});
	receiver_model.set_baseband_bandwidth(1750000);
}

AFSKRXView::~AFSKRXView() {
	receiver_model.disable();
}

void AFSKRXView::on_show() {
	auto& message_map = context().message_map();
	message_map.register_handler(Message::ID::AFSKData,
		[this](Message* const p) {
			const auto message = static_cast<const AFSKDataMessage*>(p);
			this->on_data_afsk(*message);
		}
	);
	
	receiver_model.enable();
}

void AFSKRXView::on_hide() {
	auto& message_map = context().message_map();
	message_map.unregister_handler(Message::ID::AFSKData);
}

void AFSKRXView::on_data_afsk(const AFSKDataMessage& message) {
	Coord oy,ny;
	
	//text_rx.set(to_string_dec_int(abs(message.data), 8, '0'));
	
	portapack::display.fill_rectangle({0,160-64,240,128},{32,32,32});
	
	oy = 160;
	for (int c=0;c<128;c++) {
		ny = 160-(message.data[c]>>10);
		portapack::display.draw_line({static_cast<Coord>(c),oy},{static_cast<Coord>(c+1),ny},{255,127,0});
		oy = ny;
	}
		
	/*auto payload = message.packet.payload;
	auto payload_length = message.packet.bits_received;

	std::string hex_data;
	std::string hex_error;
	uint8_t byte_data = 0;
	uint8_t byte_error = 0;
	for(size_t i=0; i<payload_length; i+=2) {
		const auto bit_data = payload[i+1];
		const auto bit_error = (payload[i+0] == payload[i+1]);

		byte_data <<= 1;
		byte_data |= bit_data ? 1 : 0;

		byte_error <<= 1;
		byte_error |= bit_error ? 1 : 0;

		if( ((i >> 1) & 7) == 7 ) {
			hex_data += to_string_hex(byte_data, 2);
			hex_error += to_string_hex(byte_error, 2);
		}
	}

	auto console = reinterpret_cast<Console*>(widget_content.get());
	console->writeln(hex_data.substr(0, 240 / 8));

	if( !f_error(&fil_tpms) ) {
		rtc::RTC datetime;
		rtcGetTime(&RTCD1, &datetime);
		std::string timestamp = 
			to_string_dec_uint(datetime.year(), 4) +
			to_string_dec_uint(datetime.month(), 2, '0') +
			to_string_dec_uint(datetime.day(), 2, '0') +
			to_string_dec_uint(datetime.hour(), 2, '0') +
			to_string_dec_uint(datetime.minute(), 2, '0') +
			to_string_dec_uint(datetime.second(), 2, '0');

		const auto tuning_frequency = receiver_model.tuning_frequency();
		// TODO: function doesn't take uint64_t, so when >= 1<<32, weirdness will ensue!
		const auto tuning_frequency_str = to_string_dec_uint(tuning_frequency, 10);

		std::string log = timestamp + " " + tuning_frequency_str + " FSK 38.4 19.2 " + hex_data + "/" + hex_error + "\r\n";
		f_puts(log.c_str(), &fil_tpms);
		f_sync(&fil_tpms);
	}*/
}

void AFSKRXView::focus() {
	button_done.focus();
}

} /* namespace ui */
