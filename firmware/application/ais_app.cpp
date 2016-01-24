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

static std::string latlon_abs_normalized(const int32_t normalized, const char suffixes[2]) {
	const auto suffix = suffixes[(normalized < 0) ? 0 : 1];
	const uint32_t normalized_abs = std::abs(normalized);
	const uint32_t t = (normalized_abs * 5) / 3;
	const uint32_t degrees = t / (100 * 10000);
	const uint32_t fraction = t % (100 * 10000);
	return to_string_dec_uint(degrees) + "." + to_string_dec_uint(fraction, 6, '0') + suffix;
}

static std::string latitude(const Latitude value) {
	if( value.is_not_available() ) {
		return "not available";
	} else if( value.is_valid() ) {
		return latlon_abs_normalized(value.normalized(), "SN");
	} else {
		return "invalid";
	}
}

static std::string longitude(const Longitude value) {
	if( value.is_not_available() ) {
		return "not available";
	} else if( value.is_valid() ) {
		return latlon_abs_normalized(value.normalized(), "WE");
	} else {
		return "invalid";
	}
}

static std::string latlon(const Latitude latitude, const Longitude longitude) {
	if( latitude.is_valid() && longitude.is_valid() ) {
		return latlon_abs_normalized(latitude.normalized(), "SN") + " " + latlon_abs_normalized(longitude.normalized(), "WE");
	} else if( latitude.is_not_available() && longitude.is_not_available() ) {
		return "not available";
	} else {
		return "invalid";
	}
}

static std::string mmsi(
	const ais::MMSI& mmsi
) {
	return to_string_dec_uint(mmsi, 9);
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
		default: return "unknown";
	}
}

static std::string rate_of_turn(const RateOfTurn value) {
	switch(value) {
	case -128: return "not available";
	case -127: return "left >5 deg/30sec";
	case    0: return "0 deg/min";
	case  127: return "right >5 deg/30sec";
	default:
		{
			std::string result = (value < 0) ? "left " : "right ";
			const float value_deg_sqrt = value / 4.733f;
			const int32_t value_deg = value_deg_sqrt * value_deg_sqrt;
			result += to_string_dec_uint(value_deg);
			result += " deg/min";
			return result;
		}
	}
}

static std::string speed_over_ground(const SpeedOverGround value) {
	if( value == 1023 ) {
		return "not available";
	} else if( value == 1022 ) {
		return ">= 102.2 knots";
	} else {
		return to_string_dec_uint(value / 10) + "." + to_string_dec_uint(value % 10, 1) + " knots";
	}
}

static std::string course_over_ground(const CourseOverGround value) {
	if( value > 3600 ) {
		return "invalid";
	} else if( value == 3600 ) {
		return "not available";
	} else {
		return to_string_dec_uint(value / 10) + "." + to_string_dec_uint(value % 10, 1) + " deg";
	}
}

static std::string true_heading(const TrueHeading value) {
	if( value == 511 ) {
		return "not available";
	} else if( value > 359 ) {
		return "invalid";
	} else {
		return to_string_dec_uint(value) + " deg";
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
		last_position.rate_of_turn = packet.read(42, 8);
		last_position.speed_over_ground = packet.read(50, 10);
		last_position.timestamp = packet.received_at();
		last_position.latitude = packet.latitude(89);
		last_position.longitude = packet.longitude(61);
		last_position.course_over_ground = packet.read(116, 12);
		last_position.true_heading = packet.read(128, 9);
		break;

	case 4:
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

namespace ui {

template<>
void RecentEntriesView<AISRecentEntries>::draw(
	const Entry& entry,
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

AISRecentEntryDetailView::AISRecentEntryDetailView() {
	add_children({ {
		&button_done,
	} });

	button_done.on_select = [this](const ui::Button&) {
		if( this->on_close ) {
			this->on_close();
		}
	};
}

void AISRecentEntryDetailView::focus() {
	button_done.focus();
}

Rect AISRecentEntryDetailView::draw_field(
	Painter& painter,
	const Rect& draw_rect,
	const Style& style,
	const std::string& label,
	const std::string& value
) {
	const int label_length_max = 4;

	painter.draw_string(Point { draw_rect.left(), draw_rect.top() }, style, label);
	painter.draw_string(Point { draw_rect.left() + (label_length_max + 1) * 8, draw_rect.top() }, style, value);

	return { draw_rect.left(), draw_rect.top() + draw_rect.height(), draw_rect.width(), draw_rect.height() };
}

void AISRecentEntryDetailView::paint(Painter& painter) {
	View::paint(painter);

	const auto s = style();
	const auto rect = screen_rect();

	auto field_rect = Rect { rect.left(), rect.top() + 16, rect.width(), 16 };

	field_rect = draw_field(painter, field_rect, s, "MMSI", ais::format::mmsi(entry_.mmsi));
	field_rect = draw_field(painter, field_rect, s, "Name", entry_.name);
	field_rect = draw_field(painter, field_rect, s, "Call", entry_.call_sign);
	field_rect = draw_field(painter, field_rect, s, "Dest", entry_.destination);
	field_rect = draw_field(painter, field_rect, s, "Last", to_string_datetime(entry_.last_position.timestamp));
	field_rect = draw_field(painter, field_rect, s, "Pos ", ais::format::latlon(entry_.last_position.latitude, entry_.last_position.longitude));
	field_rect = draw_field(painter, field_rect, s, "Stat", ais::format::navigational_status(entry_.navigational_status));
	field_rect = draw_field(painter, field_rect, s, "RoT ", ais::format::rate_of_turn(entry_.last_position.rate_of_turn));
	field_rect = draw_field(painter, field_rect, s, "SoG ", ais::format::speed_over_ground(entry_.last_position.speed_over_ground));
	field_rect = draw_field(painter, field_rect, s, "CoG ", ais::format::course_over_ground(entry_.last_position.course_over_ground));
	field_rect = draw_field(painter, field_rect, s, "Head", ais::format::true_heading(entry_.last_position.true_heading));
	field_rect = draw_field(painter, field_rect, s, "Rx #", to_string_dec_uint(entry_.received_count));
}

void AISRecentEntryDetailView::set_entry(const AISRecentEntry& entry) {
	entry_ = entry;
	set_dirty();
}

AISAppView::AISAppView() {
	add_children({ {
		&recent_entries_view,
		&recent_entry_detail_view,
	} });

	recent_entry_detail_view.hidden(true);

	EventDispatcher::message_map().register_handler(Message::ID::AISPacket,
		[this](Message* const p) {
			const auto message = static_cast<const AISPacketMessage*>(p);
			const ais::Packet packet { message->packet };
			if( packet.is_valid() ) {
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

	recent_entries_view.on_select = [this](const AISRecentEntry& entry) {
		this->on_show_detail(entry);
	};
	recent_entry_detail_view.on_close = [this]() {
		this->on_show_list();
	};
}

AISAppView::~AISAppView() {
	EventDispatcher::message_map().unregister_handler(Message::ID::AISPacket);
}

void AISAppView::set_parent_rect(const Rect new_parent_rect) {
	View::set_parent_rect(new_parent_rect);
	recent_entries_view.set_parent_rect({ 0, 0, new_parent_rect.width(), new_parent_rect.height() });
	recent_entry_detail_view.set_parent_rect({ 0, 0, new_parent_rect.width(), new_parent_rect.height() });
}

void AISAppView::on_packet(const ais::Packet& packet) {
	logger.on_packet(packet);
	const auto updated_entry = recent.on_packet(packet.source_id(), packet);
	recent_entries_view.set_dirty();

	// TODO: Crude hack, should be a more formal listener arrangement...
	if( updated_entry.key() == recent_entry_detail_view.entry().key() ) {
		recent_entry_detail_view.set_entry(updated_entry);
	}
}

void AISAppView::on_show_list() {
	recent_entries_view.hidden(false);
	recent_entry_detail_view.hidden(true);
	recent_entries_view.focus();
}

void AISAppView::on_show_detail(const AISRecentEntry& entry) {
	recent_entries_view.hidden(true);
	recent_entry_detail_view.hidden(false);
	recent_entry_detail_view.set_entry(entry);
	recent_entry_detail_view.focus();
}

} /* namespace ui */
