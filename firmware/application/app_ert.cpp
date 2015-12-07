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

#include "app_ert.hpp"

#include "portapack.hpp"
using namespace portapack;

#include "manchester.hpp"

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#include "string_format.hpp"

namespace ert {

size_t Packet::length() const {
	return payload_length_;
}

bool Packet::is_valid() const {
	return true;
}

rtc::RTC Packet::received_at() const {
	return received_at_;
}

ERTPacket::Type Packet::type() const {
	return type_;
}

ID Packet::id() const {
	if( type() == ERTPacket::Type::SCM ) {
		const auto msb = reader_.read(0, 2);
		const auto lsb = reader_.read(35, 24);
		return (msb << 24) | lsb;
	}
	if( type() == ERTPacket::Type::IDM ) {
		return reader_.read(5 * 8, 32);
	}
	return invalid_id;
}

Consumption Packet::consumption() const {
	if( type() == ERTPacket::Type::SCM ) {
		return reader_.read(11, 24);
	}
	if( type() == ERTPacket::Type::IDM ) {
		return reader_.read(25 * 8, 32);
	}
	return invalid_consumption;
}

} /* namespace ert */

ERTModel::ERTModel() {
	receiver_model.set_baseband_configuration({
		.mode = 6,
		.sampling_rate = 4194304,
		.decimation_factor = 1,
	});
	receiver_model.set_baseband_bandwidth(2500000);

	log_file.open_for_append("ert.txt");
}

bool ERTModel::on_packet(const ert::Packet&) {
	return true;
}

namespace ui {

void ERTView::on_show() {
	Console::on_show();

	auto& message_map = context().message_map();
	message_map.register_handler(Message::ID::ERTPacket,
		[this](Message* const p) {
			const auto message = static_cast<const ERTPacketMessage*>(p);
			rtc::RTC datetime;
			rtcGetTime(&RTCD1, &datetime);
			const ert::Packet packet { datetime, message->packet.type, message->packet.payload, message->packet.bits_received };
			if( this->model.on_packet(packet) ) {
				this->on_packet(packet);
			}
		}
	);
}

void ERTView::on_hide() {
	auto& message_map = context().message_map();
	message_map.unregister_handler(Message::ID::ERTPacket);

	Console::on_hide();
}

void ERTView::on_packet(const ert::Packet& packet) {
	std::string msg;
	switch(packet.type()) {
	case ERTPacket::Type::SCM:
		msg += "SCM ";
		msg += to_string_dec_uint(packet.id(), 10);
		msg += " ";
		msg += to_string_dec_uint(packet.consumption(), 10);
		break;

	case ERTPacket::Type::IDM:
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

} /* namespace ui */
