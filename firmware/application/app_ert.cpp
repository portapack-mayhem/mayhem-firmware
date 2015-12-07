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

#include "crc.hpp"
#include "string_format.hpp"

namespace ert {

size_t Packet::length() const {
	return decoder_.symbols_count();
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

ManchesterFormatted Packet::symbols_formatted() const {
	return format_manchester(decoder_);
}

bool Packet::crc_ok() const {
	switch(type()) {
	case ERTPacket::Type::SCM:	return crc_ok_scm();
	case ERTPacket::Type::IDM:	return crc_ok_idm();
	default:					return false;
	}
}

bool Packet::crc_ok_scm() const {
	CRC<uint16_t> ert_bch { 0x6f63 };
	size_t start_bit = 5;
	auto crc_calculated = ert_bch.calculate_byte(0x0000, reader_.read(0, start_bit));
	for(size_t i=start_bit; i<length(); i+=8) {
		const uint8_t byte = reader_.read(i, 8);
		crc_calculated = ert_bch.calculate_byte(crc_calculated, byte);
	}
	return crc_calculated == 0x0000;
}

bool Packet::crc_ok_idm() const {
	CRC<uint16_t> ert_crc_ccitt { 0x1021 };
	uint16_t crc_calculated = 0xffff;
	for(size_t i=0; i<length(); i+=8) {
		const uint8_t byte = reader_.read(i, 8);
		crc_calculated = ert_crc_ccitt.calculate_byte(crc_calculated, byte);
	}
	return crc_calculated == 0x1d0f;
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

bool ERTModel::on_packet(const ert::Packet& packet) {
	if( log_file.is_ready() ) {
		const auto formatted = packet.symbols_formatted();
		log_file.write_entry(packet.received_at(), formatted.data + "/" + formatted.errors);
	}

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
			const ert::Packet packet { datetime, message->packet.type, message->packet.packet };
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
		msg += packet.crc_ok() ? " *" : " x";
		break;

	case ERTPacket::Type::IDM:
		msg += "IDM ";
		msg += to_string_dec_uint(packet.id(), 10);
		msg += " ";
		msg += to_string_dec_uint(packet.consumption(), 10);
		msg += packet.crc_ok() ? " *" : " x";
		break;

	default:
		msg += "???";
		break;
	}

	writeln(msg);
}

} /* namespace ui */
