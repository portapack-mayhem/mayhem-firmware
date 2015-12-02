/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "app_tpms.hpp"

#include "portapack.hpp"
using namespace portapack;

#include <hal.h>
// #include "lpc43xx_cpp.hpp"
// using namespace lpc43xx;

// TODO: SERIOUSLY!? Including this, just to use to_string_hex?! Refactor!!!1
#include "ui_widget.hpp"

TPMSModel::TPMSModel() {
	receiver_model.set_baseband_configuration({
		.mode = 5,
		.sampling_rate = 2457600,
		.decimation_factor = 4,
	});
	receiver_model.set_baseband_bandwidth(1750000);

	open_file();
}

TPMSModel::~TPMSModel() {
	f_close(&fil);
}

ManchesterFormatted TPMSModel::on_packet(const TPMSPacketMessage& message) {
	const ManchesterDecoder decoder(message.packet.payload, message.packet.bits_received, 1);
	const auto hex_formatted = format_manchester(decoder);

	if( !f_error(&fil) ) {
		rtc::RTC datetime;
		rtcGetTime(&RTCD1, &datetime);
		std::string timestamp = 
			ui::to_string_dec_uint(datetime.year(), 4) +
			ui::to_string_dec_uint(datetime.month(), 2, '0') +
			ui::to_string_dec_uint(datetime.day(), 2, '0') +
			ui::to_string_dec_uint(datetime.hour(), 2, '0') +
			ui::to_string_dec_uint(datetime.minute(), 2, '0') +
			ui::to_string_dec_uint(datetime.second(), 2, '0');

		const auto tuning_frequency = receiver_model.tuning_frequency();
		// TODO: function doesn't take uint64_t, so when >= 1<<32, weirdness will ensue!
		const auto tuning_frequency_str = ui::to_string_dec_uint(tuning_frequency, 10);

		std::string log = timestamp + " " + tuning_frequency_str + " FSK 38.4 19.2 " + hex_formatted.data + "/" + hex_formatted.errors + "\r\n";
		f_puts(log.c_str(), &fil);
		f_sync(&fil);
	}

	return hex_formatted;
}

void TPMSModel::open_file() {
	const auto open_result = f_open(&fil, "tpms.txt", FA_WRITE | FA_OPEN_ALWAYS);
	if( open_result == FR_OK ) {
		const auto fil_size = f_size(&fil);
		const auto seek_result = f_lseek(&fil, fil_size);
		if( seek_result != FR_OK ) {
			f_close(&fil);
			// TODO: Error, indicate somehow.
		} else {
			// TODO: Indicate success.
		}
	} else {
		// TODO: Error, indicate somehow.
	}
}

namespace ui {

void TPMSView::on_show() {
	Console::on_show();

	auto& message_map = context().message_map();
	message_map.register_handler(Message::ID::TPMSPacket,
		[this](Message* const p) {
			const auto message = static_cast<const TPMSPacketMessage*>(p);
			this->log(this->model.on_packet(*message));
		}
	);
}

void TPMSView::on_hide() {
	auto& message_map = context().message_map();
	message_map.unregister_handler(Message::ID::TPMSPacket);

	Console::on_hide();
}

void TPMSView::log(const ManchesterFormatted& formatted) {
	writeln(formatted.data.substr(0, 240 / 8));
}

} /* namespace ui */
