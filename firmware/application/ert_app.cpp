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

#include "ert_app.hpp"

#include "event_m0.hpp"

#include "portapack.hpp"
using namespace portapack;

#include "manchester.hpp"

#include "crc.hpp"
#include "string_format.hpp"

void ERTLogger::on_packet(const ert::Packet& packet) {
	if( log_file.is_ready() ) {
		const auto formatted = packet.symbols_formatted();
		log_file.write_entry(packet.received_at(), formatted.data + "/" + formatted.errors);
	}
}

namespace ui {

ERTView::ERTView() {
	EventDispatcher::message_map().register_handler(Message::ID::ERTPacket,
		[this](Message* const p) {
			const auto message = static_cast<const ERTPacketMessage*>(p);
			const ert::Packet packet { message->type, message->packet };
			this->on_packet(packet);
		}
	);

	receiver_model.set_baseband_configuration({
		.mode = 6,
		.sampling_rate = 4194304,
		.decimation_factor = 1,
	});
	receiver_model.set_baseband_bandwidth(2500000);
}

ERTView::~ERTView() {
	EventDispatcher::message_map().unregister_handler(Message::ID::ERTPacket);
}

void ERTView::on_packet(const ert::Packet& packet) {
	logger.on_packet(packet);

	if( packet.crc_ok() ) {
		std::string msg;
		switch(packet.type()) {
		case ert::Packet::Type::SCM:
			msg += "SCM ";
			msg += to_string_dec_uint(packet.id(), 10);
			msg += " ";
			msg += to_string_dec_uint(packet.consumption(), 10);
			break;

		case ert::Packet::Type::IDM:
			msg += "IDM ";
			msg += to_string_dec_uint(packet.id(), 10);
			msg += " ";
			msg += to_string_dec_uint(packet.consumption(), 10);
			break;

		default:
			msg += "???";
			break;
		}

		writeln(msg);
	}
}

} /* namespace ui */
