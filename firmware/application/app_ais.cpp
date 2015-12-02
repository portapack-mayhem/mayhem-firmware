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

#include "app_ais.hpp"

#include "portapack.hpp"
using namespace portapack;

#include "crc.hpp"

// TODO: Move string formatting elsewhere!!!
#include "ui_widget.hpp"

namespace baseband {
namespace ais {

struct CRCBitRemap {
	size_t operator()(const size_t bit_index) const {
		return bit_index;
	}
};

using CRCFieldReader = ::FieldReader<std::bitset<1024>, CRCBitRemap>;

struct PacketLengthRange {
	constexpr PacketLengthRange(
	) : min_bytes { 0 },
		max_bytes { 0 }
	{
	}

	constexpr PacketLengthRange(
		const uint16_t min_bits,
		const uint16_t max_bits
	) : min_bytes { static_cast<uint8_t>(min_bits / 8U) },
		max_bytes { static_cast<uint8_t>(max_bits / 8U) }
	{
		// static_assert((min_bits & 7) == 0, "minimum bits not a multiple of 8");
		// static_assert((max_bits & 7) == 0, "minimum bits not a multiple of 8");
	}

	bool contains(const size_t bit_count) const {
		return !is_above(bit_count) && !is_below(bit_count);
	}

	bool is_above(const size_t bit_count) const {
		return (min() > bit_count);
	}

	bool is_below(const size_t bit_count) const {
		return (max() < bit_count);
	}

	size_t min() const {
		return min_bytes * 8;
	}
	
	size_t max() const {
		return max_bytes * 8;
	}

private:
	const uint8_t min_bytes;
	const uint8_t max_bytes;
};

static constexpr std::array<PacketLengthRange, 64> packet_length_range { {
	{    0,    0 },	// 0
	{  168,  168 },	// 1
	{  168,  168 },	// 2
	{  168,  168 }, // 3
	{  168,  168 }, // 4
	{  424,  424 }, // 5
	{    0,    0 }, // 6
	{    0,    0 }, // 7
	{    0, 1008 }, // 8
	{    0,    0 }, // 9
	{    0,    0 }, // 10
	{    0,    0 }, // 11
	{    0,    0 }, // 12
	{    0,    0 }, // 13
	{    0,    0 }, // 14
	{    0,    0 }, // 15
	{    0,    0 }, // 16
	{    0,    0 }, // 17
	{  168,  168 }, // 18
	{    0,    0 }, // 19
	{   72,  160 }, // 20
	{  272,  360 }, // 21
	{  168,  168 }, // 22
	{  160,  160 }, // 23
	{  160,  168 }, // 24
	{    0,  168 }, // 25
	{    0,    0 }, // 26
	{    0,    0 }, // 27
	{    0,    0 }, // 28
	{    0,    0 }, // 29
	{    0,    0 }, // 30
	{    0,    0 }, // 31
} };

struct PacketLengthValidator {
	bool operator()(const uint_fast8_t message_id, const size_t length) {
		return packet_length_range[message_id].contains(length);
	}
};

struct PacketTooLong {
	bool operator()(const uint_fast8_t message_id, const size_t length) {
		return packet_length_range[message_id].is_below(length);
	}
};

struct CRCCheck {
	bool operator()(const std::bitset<1024>& payload, const size_t data_length) {
		CRCFieldReader field_crc { payload };
		CRC<uint16_t> ais_fcs { 0x1021 };
		
		uint16_t crc_calculated = 0xffff;
		
		for(size_t i=0; i<data_length + 16; i+=8) {
			if( i == data_length ) {
				crc_calculated ^= 0xffff;
			}
			const uint8_t byte = field_crc.read(i, 8);
			crc_calculated = ais_fcs.calculate_byte(crc_calculated, byte);
		}

		return crc_calculated == 0x0000;
	}
};

static std::string format_latlon_normalized(const int32_t normalized) {
	const int32_t t = (normalized * 5) / 3;
	const int32_t degrees = t / (100 * 10000);
	const int32_t fraction = std::abs(t) % (100 * 10000);
	return ui::to_string_dec_int(degrees) + "." + ui::to_string_dec_int(fraction, 6, '0');
}

static std::string format_datetime(
	const DateTime& datetime
) {
	return ui::to_string_dec_uint(datetime.year, 4) + "/" +
		ui::to_string_dec_uint(datetime.month, 2, '0') + "/" +
		ui::to_string_dec_uint(datetime.day, 2, '0') + " " +
		ui::to_string_dec_uint(datetime.hour, 2, '0') + ":" +
		ui::to_string_dec_uint(datetime.minute, 2, '0') + ":" +
		ui::to_string_dec_uint(datetime.second, 2, '0');
}

static std::string format_navigational_status(const unsigned int value) {
	switch(value) {
		case 0: return "under way w/engine";
		case 1: return "at anchor";
		case 2: return "not under command";
		case 3: return "restricted maneuv";
		case 4: return "constrained draught";
		case 5: return "moored";
		case 6: return "aground";
		case 7: return "fishing";
		case 8: return "sailing";
		case 9: case 10: case 13: return "reserved";
		case 11: return "towing astern";
		case 12: return "towing ahead/along";
		case 14: return "SART/MOB/EPIRB";
		case 15: return "undefined";
		default: return "unexpected";
	}
}

static char char_to_ascii(const uint8_t c) {
	return (c ^ 32) + 32;
}

size_t Packet::length() const {
	return payload_length_;
}

bool Packet::is_valid() const {
	// Subtract end flag (8 bits) - one unstuffing bit (occurs during end flag).
	const size_t data_and_fcs_length = payload_length_ - 7;

	if( data_and_fcs_length < 38 ) {
		return false;
	}

	const size_t extra_bits = data_and_fcs_length & 7;
	if( extra_bits != 0 ) {
		return false;
	}

	const size_t data_length = data_and_fcs_length - 16;
	PacketLengthValidator packet_length_valid;
	if( !packet_length_valid(message_id(), data_length) ) {
		return false;
	}

	CRCCheck crc_ok;
	if( !crc_ok(payload_, data_length) ) {
		return false;
	}

	return true;
}

uint32_t Packet::message_id() const {
	return field_.read(0, 6);
}

uint32_t Packet::source_id() const {
	return field_.read(8, 30);
}

uint32_t Packet::read(const size_t start_bit, const size_t length) const {
	return field_.read(start_bit, length);
}

std::string Packet::text(
	const size_t start_bit,
	const size_t character_count
) const {
	std::string result;
	const size_t character_length = 6;
	const size_t end_bit = start_bit + character_count * character_length;
	for(size_t i=start_bit; i<end_bit; i+=character_length) {
		result += char_to_ascii(field_.read(i, character_length));
	} 

	return result;
}

DateTime Packet::datetime(const size_t start_bit) const {
	return {
		static_cast<uint16_t>(field_.read(start_bit +  0, 14)),
		static_cast<uint8_t >(field_.read(start_bit + 14,  4)),
		static_cast<uint8_t >(field_.read(start_bit + 18,  5)),
		static_cast<uint8_t >(field_.read(start_bit + 23,  5)),
		static_cast<uint8_t >(field_.read(start_bit + 28,  6)),
		static_cast<uint8_t >(field_.read(start_bit + 34,  6)),
	};
}

Latitude Packet::latitude(const size_t start_bit) const {
	// Shifting and dividing is to sign-extend the source field.
	// TODO: There's probably a more elegant way to do it.
	return static_cast<int32_t>(field_.read(start_bit, 27) << 5) / 32;
}

Longitude Packet::longitude(const size_t start_bit) const {
	// Shifting and dividing is to sign-extend the source field.
	// TODO: There's probably a more elegant way to do it.
	return static_cast<int32_t>(field_.read(start_bit, 28) << 4) / 16;
}

} /* namespace ais */
} /* namespace baseband */

AISModel::AISModel() {
	receiver_model.set_baseband_configuration({
		.mode = 3,
		.sampling_rate = 2457600,
		.decimation_factor = 4,
	});
	receiver_model.set_baseband_bandwidth(1750000);

	log_file.open_for_append("ais.txt");
}

bool AISModel::on_packet(const baseband::ais::Packet& packet) {
	// TODO: Unstuff here, not in baseband!

	if( !packet.is_valid() ) {
		return false;
	}

	if( log_file.is_ready() ) {
		std::string entry;
		entry.reserve((packet.length() + 3) / 4);

		for(size_t i=0; i<packet.length(); i+=4) {
			const auto nibble = packet.read(i, 4);
			entry += (nibble >= 10) ? ('W' + nibble) : ('0' + nibble);
		}

		entry += "\r\n";
		log_file.write(entry);
	}

	return true;
}	

namespace ui {

void AISView::on_show() {
	Console::on_show();

	auto& message_map = context().message_map();
	message_map.register_handler(Message::ID::AISPacket,
		[this](Message* const p) {
			const auto message = static_cast<const AISPacketMessage*>(p);
			const baseband::ais::Packet packet { message->packet.payload, message->packet.bits_received };
			if( this->model.on_packet(packet) ) {
				this->log(packet);
			}
		}
	);
}

void AISView::on_hide() {
	auto& message_map = context().message_map();
	message_map.unregister_handler(Message::ID::AISPacket);

	Console::on_hide();
}

void AISView::log(const baseband::ais::Packet& packet) {
	std::string result { ui::to_string_dec_uint(packet.message_id(), 2) + " " + ui::to_string_dec_uint(packet.source_id(), 10) };

	switch(packet.message_id()) {
	case 1:
	case 2:
	case 3:
		{
			const auto navigational_status = packet.read(38, 4);
			result += " " + baseband::ais::format_navigational_status(navigational_status);
			result += " " + baseband::ais::format_latlon_normalized(packet.latitude(89));
			result += " " + baseband::ais::format_latlon_normalized(packet.longitude(61));
		}
		break;

	case 4:
		{
			result += " " + baseband::ais::format_datetime(packet.datetime(38));
			result += " " + baseband::ais::format_latlon_normalized(packet.latitude(107));
			result += " " + baseband::ais::format_latlon_normalized(packet.longitude(79));
		}
		break;

	case 5:
		{
			const auto call_sign = packet.text(70, 7);
			const auto name = packet.text(112, 20);
			const auto destination = packet.text(302, 20);
			result += " \"" + call_sign + "\" \"" + name + "\" \"" + destination + "\""; 
		}
		break;

	case 21:
		{
			const auto name = packet.text(43, 20);
			result += " \"" + name + "\" " + baseband::ais::format_latlon_normalized(packet.latitude(192)) + " " + baseband::ais::format_latlon_normalized(packet.longitude(164));
		}
		break;

	default:
		break;
	}

	writeln(result);
}

} /* namespace ui */
