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

#include "tpms_app.hpp"

#include "event_m0.hpp"

#include "portapack.hpp"
using namespace portapack;

#include "string_format.hpp"

tpms::Packet TPMSModel::on_packet(const TPMSPacketMessage& message) {
	const ManchesterDecoder decoder(message.packet, 1);
	const auto hex_formatted = format_manchester(decoder);

	if( log_file.is_ready() ) {
		const auto tuning_frequency = receiver_model.tuning_frequency();
		// TODO: function doesn't take uint64_t, so when >= 1<<32, weirdness will ensue!
		const auto tuning_frequency_str = to_string_dec_uint(tuning_frequency, 10);

		std::string entry = tuning_frequency_str + " FSK 38.4 19.2 " + hex_formatted.data + "/" + hex_formatted.errors;
		log_file.write_entry(message.packet.timestamp(), entry);
	}

	return hex_formatted;
}

namespace ui {

TPMSAppView::TPMSAppView() {
	add_children({ {
		&console,
	} });

	EventDispatcher::message_map().register_handler(Message::ID::TPMSPacket,
		[this](Message* const p) {
			const auto message = static_cast<const TPMSPacketMessage*>(p);
			this->log(this->model.on_packet(*message));
		}
	);

	receiver_model.set_baseband_configuration({
		.mode = 5,
		.sampling_rate = 2457600,
		.decimation_factor = 1,
	});
	receiver_model.set_baseband_bandwidth(1750000);
}

TPMSAppView::~TPMSAppView() {
	EventDispatcher::message_map().unregister_handler(Message::ID::TPMSPacket);
}

void TPMSAppView::set_parent_rect(const Rect new_parent_rect) {
	View::set_parent_rect(new_parent_rect);
	console.set_parent_rect({ 0, 0, new_parent_rect.width(), new_parent_rect.height() });
}

void TPMSAppView::log(const tpms::Packet& formatted) {
	console.writeln(formatted.data.substr(0, 240 / 8));
}

} /* namespace ui */
