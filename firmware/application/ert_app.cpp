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

#include "baseband_api.hpp"

#include "manchester.hpp"

#include "crc.hpp"
#include "string_format.hpp"

namespace ert {

namespace format {

std::string type(Packet::Type value) {
	switch(value) {
	default:
	case Packet::Type::Unknown:	return "???";
	case Packet::Type::IDM:		return "IDM";
	case Packet::Type::SCM:		return "SCM";
	}
}

std::string id(ID value) {
	return to_string_dec_uint(value, 10);
}

std::string consumption(Consumption value) {
	return to_string_dec_uint(value, 10);
}

std::string commodity_type(CommodityType value) {
	return to_string_dec_uint(value, 2);
}

} /* namespace format */

} /* namespace ert */

ERTLogger::ERTLogger(
	const std::string& file_path
) : log_file { file_path }
{
}

void ERTLogger::on_packet(const ert::Packet& packet) {
	if( log_file.is_open() ) {
		const auto formatted = packet.symbols_formatted();
		log_file.write_entry(packet.received_at(), formatted.data + "/" + formatted.errors);
	}
}

const ERTRecentEntry::Key ERTRecentEntry::invalid_key { };

void ERTRecentEntry::update(const ert::Packet& packet) {
	received_count++;

	last_consumption = packet.consumption();
}

namespace ui {

static const std::array<std::pair<std::string, size_t>, 4> ert_columns { {
	{ "ID", 10 },
	{ "Tp", 2 },
	{ "Consumpt", 10 },
	{ "Cnt", 3 },
} };

template<>
void RecentEntriesView<ERTRecentEntries>::draw_header(
	const Rect& target_rect,
	Painter& painter,
	const Style& style
) {
	auto x = 0;
	for(const auto& column : ert_columns) {
		const auto width = column.second;
		auto text = column.first;
		if( width > text.length() ) {
			text.append(width - text.length(), ' ');
		}

		painter.draw_string({ x, target_rect.pos.y }, style, text);
		x += (width * 8) + 8;
	}
}

template<>
void RecentEntriesView<ERTRecentEntries>::draw(
	const Entry& entry,
	const Rect& target_rect,
	Painter& painter,
	const Style& style,
	const bool is_selected
) {
	const auto& draw_style = is_selected ? style.invert() : style;

	std::string line = ert::format::id(entry.id) + " " + ert::format::commodity_type(entry.commodity_type) + " " + ert::format::consumption(entry.last_consumption);

	if( entry.received_count > 999 ) {
		line += " +++";
	} else {
		line += " " + to_string_dec_uint(entry.received_count, 3);
	}

	line.resize(target_rect.width() / 8, ' ');
	painter.draw_string(target_rect.pos, draw_style, line);
}

ERTAppView::ERTAppView(NavigationView&) {
	add_children({ {
		&recent_entries_view,
	} });

	EventDispatcher::message_map().register_handler(Message::ID::ERTPacket,
		[this](Message* const p) {
			const auto message = static_cast<const ERTPacketMessage*>(p);
			const ert::Packet packet { message->type, message->packet };
			this->on_packet(packet);
		}
	);

	radio::enable({
		initial_target_frequency,
		sampling_rate,
		baseband_bandwidth,
		rf::Direction::Receive,
		false, 32, 32,
		1,
	});

	baseband::start({
		.mode = 6,
		.sampling_rate = sampling_rate,
		.decimation_factor = 1,
	});

	logger = std::make_unique<ERTLogger>("ert.txt");
}

ERTAppView::~ERTAppView() {
	baseband::stop();
	radio::disable();

	EventDispatcher::message_map().unregister_handler(Message::ID::ERTPacket);
}

void ERTAppView::focus() {
	recent_entries_view.focus();
}

void ERTAppView::set_parent_rect(const Rect new_parent_rect) {
	View::set_parent_rect(new_parent_rect);
	recent_entries_view.set_parent_rect({ 0, 0, new_parent_rect.width(), new_parent_rect.height() });
}

void ERTAppView::on_packet(const ert::Packet& packet) {
	if( logger ) {
		logger->on_packet(packet);
	}

	if( packet.crc_ok() ) {
		recent.on_packet({ packet.id(), packet.commodity_type() }, packet);
		recent_entries_view.set_dirty();
	}
}

void ERTAppView::on_show_list() {
	recent_entries_view.hidden(false);
	recent_entries_view.focus();
}

} /* namespace ui */
