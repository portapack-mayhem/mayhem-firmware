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

#include "string_format.hpp"

#include <algorithm>

namespace baseband {
namespace ais {

struct CRCBitRemap {
	size_t operator()(const size_t bit_index) const {
		return bit_index;
	}
};

using CRCFieldReader = ::FieldReader<::Packet, CRCBitRemap>;

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

static std::string format_latlon_normalized(const int32_t normalized) {
	const int32_t t = (normalized * 5) / 3;
	const int32_t degrees = t / (100 * 10000);
	const int32_t fraction = std::abs(t) % (100 * 10000);
	return to_string_dec_int(degrees) + "." + to_string_dec_int(fraction, 6, '0');
}

static std::string format_mmsi(
	const MMSI& mmsi
) {
	return to_string_dec_uint(mmsi, 9, '0');
}

static std::string format_datetime(
	const DateTime& datetime
) {
	return to_string_dec_uint(datetime.year, 4, '0') + "/" +
		to_string_dec_uint(datetime.month, 2, '0') + "/" +
		to_string_dec_uint(datetime.day, 2, '0') + " " +
		to_string_dec_uint(datetime.hour, 2, '0') + ":" +
		to_string_dec_uint(datetime.minute, 2, '0') + ":" +
		to_string_dec_uint(datetime.second, 2, '0');
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
	return packet_.size();
}

bool Packet::is_valid() const {
	if( data_and_fcs_length() < 38 ) {
		return false;
	}

	const size_t extra_bits = data_and_fcs_length() & 7;
	if( extra_bits != 0 ) {
		return false;
	}

	PacketLengthValidator packet_length_valid;
	if( !packet_length_valid(message_id(), data_length()) ) {
		return false;
	}

	return crc_ok();
}

rtc::RTC Packet::received_at() const {
	return received_at_;
}

uint32_t Packet::message_id() const {
	return field_.read(0, 6);
}

MMSI Packet::user_id() const {
	return field_.read(8, 30);
}

MMSI Packet::source_id() const {
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
	result.reserve(character_count);
	
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

bool Packet::crc_ok() const {
	CRCFieldReader field_crc { packet_ };
	CRC<uint16_t> ais_fcs { 0x1021, 0xffff, 0xffff };
	
	for(size_t i=0; i<data_length(); i+=8) {
		ais_fcs.process_byte(field_crc.read(i, 8));
	}

	return (ais_fcs.checksum() == field_crc.read(data_length(), fcs_length));
}

size_t Packet::data_and_fcs_length() const {
	// Subtract end flag (8 bits) - one unstuffing bit (occurs during end flag).
	return length() - 7;
}

size_t Packet::data_length() const {
	return data_and_fcs_length() - fcs_length;
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

		log_file.write_entry(packet.received_at(), entry);
	}

	return true;
}	

namespace ui {

void AISView::on_show() {
	View::on_show();

	auto& message_map = context().message_map();
	message_map.register_handler(Message::ID::AISPacket,
		[this](Message* const p) {
			const auto message = static_cast<const AISPacketMessage*>(p);
			rtc::RTC datetime;
			rtcGetTime(&RTCD1, &datetime);
			const baseband::ais::Packet packet { datetime, message->packet.packet };
			if( this->model.on_packet(packet) ) {
				this->on_packet(packet);
			}
		}
	);
}

void AISView::on_hide() {
	auto& message_map = context().message_map();
	message_map.unregister_handler(Message::ID::AISPacket);

	View::on_hide();
}

void AISView::truncate_entries() {
	while(recent.size() > 64) {
		recent.pop_back();
	}
}

void AISView::on_packet(const baseband::ais::Packet& packet) {
	const auto source_id = packet.source_id();
	auto matching_recent = std::find_if(recent.begin(), recent.end(),
		[source_id](const AISView::RecentEntry& entry) { return entry.mmsi == source_id; }
	);
	if( matching_recent != recent.end() ) {
		// Found within. Move to front of list, increment counter.
		recent.push_front(*matching_recent);
		recent.erase(matching_recent);
	} else {
		recent.emplace_front(source_id);
		truncate_entries();
	}

	auto& entry = recent.front();
	entry.received_count++;

	switch(packet.message_id()) {
	case 1:
	case 2:
	case 3:
		entry.navigational_status = packet.read(38, 4);
		entry.last_position.timestamp = packet.received_at();
		entry.last_position.latitude = packet.latitude(89);
		entry.last_position.longitude = packet.longitude(61);
		break;

	case 4:
		// packet.datetime(38)
		entry.last_position.timestamp = packet.received_at();
		entry.last_position.latitude = packet.latitude(107);
		entry.last_position.longitude = packet.longitude(79);
		break;

	case 5:
		entry.call_sign = packet.text(70, 7);
		entry.name = packet.text(112, 20);
		entry.destination = packet.text(302, 20);
		break;

	case 21:
		entry.name = packet.text(43, 20);
		entry.last_position.timestamp = packet.received_at();
		entry.last_position.latitude = packet.latitude(192);
		entry.last_position.longitude = packet.longitude(164);
		break;

	default:
		break;
	}

	set_dirty();
}

void AISView::on_focus() {
	has_focus = true;
	set_dirty();
}

void AISView::on_blur() {
	has_focus = false;
	set_dirty();
}

bool AISView::on_encoder(const EncoderEvent event) {
	advance(event);
	return true;
}

void AISView::draw_entry(
	const RecentEntry& entry,
	const Rect& target_rect,
	Painter& painter,
	const Style& s
) {
	std::string line = baseband::ais::format_mmsi(entry.mmsi) + " ";
	if( !entry.name.empty() ) {
		line += entry.name;
	} else {
		line += entry.call_sign;
	}

	line.resize(target_rect.width() / 8, ' ');
	painter.draw_string(target_rect.pos, s, line);
}

void AISView::paint(Painter& painter) {
	const auto r = screen_rect();
	const auto& s = style();

	Rect target_rect { r.pos, { r.width(), s.font.line_height() }};
	bool found_selected_item = false;
	for(const auto entry : recent) {
		const auto next_y = target_rect.pos.y + target_rect.height();
		const auto last_visible_entry = (next_y >= r.bottom());

		const auto is_selected_key = (selected_key == entry.mmsi);
		found_selected_item |= is_selected_key;

		if( !last_visible_entry || (last_visible_entry && found_selected_item) ) {
			const auto& draw_style = (has_focus && is_selected_key) ? s.invert() : s;
			draw_entry(entry, target_rect, painter, draw_style);
			target_rect.pos.y += target_rect.height();
		}

		if( last_visible_entry && found_selected_item ) {
			break;
		}
	}
}

AISView::RecentEntries::iterator AISView::selected_entry() {
	const auto key = selected_key;
	return std::find_if(std::begin(recent), std::end(recent), [key](const RecentEntry& e) { return e.mmsi == key; });
}

void AISView::advance(const int32_t amount) {
	auto selected = selected_entry();
	if( selected == std::end(recent) ) {
		if( recent.empty() ) {
			selected_key = invalid_key;
		} else {
			selected_key = recent.front().mmsi;
		}
	} else {
		if( amount < 0 ) {
			if( selected != std::begin(recent) ) {
				std::advance(selected, -1);
			}
		}
		if( amount > 0 ) {
			std::advance(selected, 1);
			if( selected == std::end(recent) ) {
				return;
			}
		}
		selected_key = selected->mmsi;
	}

	set_dirty();
}

} /* namespace ui */
