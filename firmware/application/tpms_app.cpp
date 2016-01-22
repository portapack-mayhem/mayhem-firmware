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

#include "crc.hpp"

#include "utility.hpp"

namespace tpms {

Timestamp Packet::received_at() const {
	return packet_.timestamp();
}

ManchesterFormatted Packet::symbols_formatted() const {
	return format_manchester(decoder_);
}

Optional<Reading> Packet::reading() const {
	const auto length = crc_valid_length();

	switch(length) {
	case 64:
		return Reading {
			Reading::Type::FLM_64,
			reader_.read(0, 32),
			Pressure { static_cast<int32_t>(reader_.read(32, 8)) * 4 / 3 },
			Temperature { static_cast<int32_t>(reader_.read(40, 8) & 0x7f) - 50 }
		};

	case 72:
		return Reading {
			Reading::Type::FLM_72,
			reader_.read(0, 32),
			Pressure { static_cast<int32_t>(reader_.read(40, 8)) * 4 / 3 },
			Temperature { static_cast<int32_t>(reader_.read(48, 8)) - 50 }
		};

	case 80:
		return Reading {
			Reading::Type::FLM_80,
			reader_.read(8, 32),
			Pressure { static_cast<int32_t>(reader_.read(48, 8)) * 4 / 3 },
			Temperature { static_cast<int32_t>(reader_.read(56, 8)) - 50 }
		};

	default:
		return { };
	}
}

size_t Packet::crc_valid_length() const {
	constexpr uint32_t checksum_bytes = 0b1111111;
	constexpr uint32_t crc_72_bytes = 0b111111111;
	constexpr uint32_t crc_80_bytes = 0b1111111110;

	std::array<uint8_t, 10> bytes;
	for(size_t i=0; i<bytes.size(); i++) {
		bytes[i] = reader_.read(i * 8, 8);
	}

	uint32_t checksum = 0;
	CRC<uint8_t> crc_72 { 0x01, 0x00 };
	CRC<uint8_t> crc_80 { 0x01, 0x00 };

	for(size_t i=0; i<bytes.size(); i++) {
		const uint32_t byte_mask = 1 << i;
		const auto byte = bytes[i];

		if( checksum_bytes & byte_mask ) {
			checksum += byte;
		}
		if( crc_72_bytes & byte_mask ) {
			crc_72.process_byte(byte);
		}
		if( crc_80_bytes & byte_mask ) {
			crc_80.process_byte(byte);
		}
	}

	if( crc_80.checksum() == 0 ) {
		return 80;
	} else if( crc_72.checksum() == 0 ) {
		return 72;
	} else if( (checksum & 0xff) == bytes[7] ) {
		return 64;
	} else {
		return 0;
	}
}

} /* namespace tpms */

void TPMSLogger::on_packet(const tpms::Packet& packet) {
	const auto hex_formatted = packet.symbols_formatted();

	if( log_file.is_ready() ) {
		const auto tuning_frequency = receiver_model.tuning_frequency();
		// TODO: function doesn't take uint64_t, so when >= 1<<32, weirdness will ensue!
		const auto tuning_frequency_str = to_string_dec_uint(tuning_frequency, 10);

		std::string entry = tuning_frequency_str + " FSK 38.4 19.2 " + hex_formatted.data + "/" + hex_formatted.errors;
		log_file.write_entry(packet.received_at(), entry);
	}
}

namespace ui {

TPMSAppView::TPMSAppView() {
	add_children({ {
		&console,
	} });

	EventDispatcher::message_map().register_handler(Message::ID::TPMSPacket,
		[this](Message* const p) {
			const auto message = static_cast<const TPMSPacketMessage*>(p);
			const tpms::Packet packet { message->packet };
			this->on_packet(packet);
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

void TPMSAppView::on_packet(const tpms::Packet& packet) {
	this->logger.on_packet(packet);
	this->draw(packet);
}

void TPMSAppView::draw(const tpms::Packet& packet) {
	const auto reading_opt = packet.reading();
	if( reading_opt.is_valid() ) {
		const auto reading = reading_opt.value();
		auto s = to_string_dec_uint(toUType(reading.type()), 2) + " " + to_string_hex(reading.id().value(), 8);
		if( reading.pressure().is_valid() ) {
			s += " " + to_string_dec_int(reading.pressure().value().kilopascal(), 3);
		}
		if( reading.temperature().is_valid() ) {
			s += " " + to_string_dec_int(reading.temperature().value().celsius(), 3);
		}
		console.writeln(s);
	}
}

} /* namespace ui */
