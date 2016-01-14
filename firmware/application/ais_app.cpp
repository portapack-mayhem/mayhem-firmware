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

#include "ais_app.hpp"

#include "event_m0.hpp"

#include "string_format.hpp"

#include "portapack.hpp"
using namespace portapack;

#include <algorithm>

namespace ais {
namespace format {

static std::string latlon_normalized(const int32_t normalized) {
	const int32_t t = (normalized * 5) / 3;
	const int32_t degrees = t / (100 * 10000);
	const int32_t fraction = std::abs(t) % (100 * 10000);
	return to_string_dec_int(degrees) + "." + to_string_dec_int(fraction, 6, '0');
}

static std::string mmsi(
	const ais::MMSI& mmsi
) {
	return to_string_dec_uint(mmsi, 9, '0');
}

static std::string datetime(
	const ais::DateTime& datetime
) {
	return to_string_dec_uint(datetime.year, 4, '0') + "/" +
		to_string_dec_uint(datetime.month, 2, '0') + "/" +
		to_string_dec_uint(datetime.day, 2, '0') + " " +
		to_string_dec_uint(datetime.hour, 2, '0') + ":" +
		to_string_dec_uint(datetime.minute, 2, '0') + ":" +
		to_string_dec_uint(datetime.second, 2, '0');
}

static std::string navigational_status(const unsigned int value) {
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

} /* namespace format */
} /* namespace ais */

void AISLogger::on_packet(const ais::Packet& packet) {
	// TODO: Unstuff here, not in baseband!
	if( log_file.is_ready() ) {
		std::string entry;
		entry.reserve((packet.length() + 3) / 4);

		for(size_t i=0; i<packet.length(); i+=4) {
			const auto nibble = packet.read(i, 4);
			entry += (nibble >= 10) ? ('W' + nibble) : ('0' + nibble);
		}

		log_file.write_entry(packet.received_at(), entry);
	}
}	

void AISRecentEntry::update(const ais::Packet& packet) {
	received_count++;

	switch(packet.message_id()) {
	case 1:
	case 2:
	case 3:
		navigational_status = packet.read(38, 4);
		last_position.timestamp = packet.received_at();
		last_position.latitude = packet.latitude(89);
		last_position.longitude = packet.longitude(61);
		break;

	case 4:
		// packet.datetime(38)
		last_position.timestamp = packet.received_at();
		last_position.latitude = packet.latitude(107);
		last_position.longitude = packet.longitude(79);
		break;

	case 5:
		call_sign = packet.text(70, 7);
		name = packet.text(112, 20);
		destination = packet.text(302, 20);
		break;

	case 21:
		name = packet.text(43, 20);
		last_position.timestamp = packet.received_at();
		last_position.latitude = packet.latitude(192);
		last_position.longitude = packet.longitude(164);
		break;

	default:
		break;
	}
}

void AISRecentEntries::on_packet(const ais::Packet& packet) {
	const auto source_id = packet.source_id();
	auto matching_recent = find_by_mmsi(source_id);
	if( matching_recent != std::end(entries) ) {
		// Found within. Move to front of list, increment counter.
		entries.push_front(*matching_recent);
		entries.erase(matching_recent);
	} else {
		entries.emplace_front(source_id);
		truncate_entries();
	}

	entries.front().update(packet);
}

AISRecentEntries::ContainerType::const_iterator AISRecentEntries::find_by_mmsi(const ais::MMSI key) const {
	return std::find_if(
		std::begin(entries), std::end(entries),
		[key](const AISRecentEntry& e) { return e.mmsi == key; }
	);
}

void AISRecentEntries::truncate_entries() {
	while(entries.size() > entries_max) {
		entries.pop_back();
	}
}

namespace ui {

AISRecentEntriesView::AISRecentEntriesView() {
	flags.focusable = true;

	EventDispatcher::message_map().register_handler(Message::ID::AISPacket,
		[this](Message* const p) {
			const auto message = static_cast<const AISPacketMessage*>(p);
			const ais::Packet packet { message->packet };
			if( packet.is_valid() ) {
				this->logger.on_packet(packet);
				this->on_packet(packet);
			}
		}
	);

	receiver_model.set_baseband_configuration({
		.mode = 3,
		.sampling_rate = 2457600,
		.decimation_factor = 1,
	});
	receiver_model.set_baseband_bandwidth(1750000);
}

AISRecentEntriesView::~AISRecentEntriesView() {
	EventDispatcher::message_map().unregister_handler(Message::ID::AISPacket);
}

void AISRecentEntriesView::on_packet(const ais::Packet& packet) {
	recent.on_packet(packet);

	set_dirty();
}

void AISRecentEntriesView::on_focus() {
	has_focus = true;
	set_dirty();
}

void AISRecentEntriesView::on_blur() {
	has_focus = false;
	set_dirty();
}

bool AISRecentEntriesView::on_encoder(const EncoderEvent event) {
	advance(event);
	return true;
}

static void ais_list_item_draw(
	const AISRecentEntry& entry,
	const Rect& target_rect,
	Painter& painter,
	const Style& style,
	const bool is_selected
) {
	const auto& draw_style = is_selected ? style.invert() : style;

	std::string line = ais::format::mmsi(entry.mmsi) + " ";
	if( !entry.name.empty() ) {
		line += entry.name;
	} else {
		line += entry.call_sign;
	}

	line.resize(target_rect.width() / 8, ' ');
	painter.draw_string(target_rect.pos, draw_style, line);
}

void AISRecentEntriesView::paint(Painter& painter) {
	const auto r = screen_rect();
	const auto& s = style();

	Rect target_rect { r.pos, { r.width(), s.font.line_height() }};
	const size_t visible_item_count = r.height() / s.font.line_height();

	auto selected = recent.find_by_mmsi(selected_key);
	if( selected == std::end(recent) ) {
		selected = std::begin(recent);
	}

	auto start = selected;
	auto end = selected;
	size_t i = 0;

	// Move start iterator toward first entry.
	while( (start != std::begin(recent)) && (i < visible_item_count / 2) ) {
		std::advance(start, -1);
		i++;
	}

	// Move end iterator toward last entry.
	while( (end != std::end(recent)) && (i < visible_item_count) ) {
		std::advance(end, 1);
		i++;
	}

	for(auto p = start; p != end; p++) {
		const auto& entry = *p;
		const auto is_selected_key = (selected_key == entry.mmsi);
		ais_list_item_draw(entry, target_rect, painter, s, (has_focus && is_selected_key));
		target_rect.pos.y += target_rect.height();
	}
}

void AISRecentEntriesView::advance(const int32_t amount) {
	auto selected = recent.find_by_mmsi(selected_key);
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
