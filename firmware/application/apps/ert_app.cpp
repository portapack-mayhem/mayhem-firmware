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

#include "baseband_api.hpp"

#include "portapack.hpp"
using namespace portapack;

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

void ERTLogger::on_packet(const ert::Packet& packet) {
	const auto formatted = packet.symbols_formatted();
	log_file.write_entry(packet.received_at(), formatted.data + "/" + formatted.errors);
}

const ERTRecentEntry::Key ERTRecentEntry::invalid_key { };

void ERTRecentEntry::update(const ert::Packet& packet) {
	received_count++;

	last_consumption = packet.consumption();
}

namespace ui {

template<>
void RecentEntriesTable<ERTRecentEntries>::draw(
	const Entry& entry,
	const Rect& target_rect,
	Painter& painter,
	const Style& style
) {
	std::string line = ert::format::id(entry.id) + " " + ert::format::commodity_type(entry.commodity_type) + " " + ert::format::consumption(entry.last_consumption);

	if( entry.received_count > 999 ) {
		line += " +++";
	} else {
		line += " " + to_string_dec_uint(entry.received_count, 3);
	}

	line.resize(target_rect.width() / 8, ' ');
	painter.draw_string(target_rect.location(), style, line);
}

ERTAppView::ERTAppView(NavigationView&) {
	baseband::run_image(portapack::spi_flash::image_tag_ert);

	add_children({
		&field_rf_amp,
		&field_lna,
		&field_vga,
		&rssi,
		&recent_entries_view,
	});

	// load app settings
	auto rc = settings.load("rx_ert", &app_settings);
	if(rc == SETTINGS_OK) {
		field_lna.set_value(app_settings.lna);
		field_vga.set_value(app_settings.vga);
		field_rf_amp.set_value(app_settings.rx_amp);
	}


	radio::enable({
		initial_target_frequency,
		sampling_rate,
		baseband_bandwidth,
		rf::Direction::Receive,
		receiver_model.rf_amp(),
		static_cast<int8_t>(receiver_model.lna()),
		static_cast<int8_t>(receiver_model.vga()),
	});

	logger = std::make_unique<ERTLogger>();
	if( logger ) {
		logger->append(u"ert.txt");
	}
}

ERTAppView::~ERTAppView() {

	// save app settings
	settings.save("rx_ert", &app_settings);

	radio::disable();

	baseband::shutdown();
}

void ERTAppView::focus() {
	field_vga.focus();
}

void ERTAppView::set_parent_rect(const Rect new_parent_rect) {
	View::set_parent_rect(new_parent_rect);
	recent_entries_view.set_parent_rect({ 0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height });
}

void ERTAppView::on_packet(const ert::Packet& packet) {
	if( logger ) {
		logger->on_packet(packet);
	}

	if( packet.crc_ok() ) {
		auto& entry = ::on_packet(recent, ERTRecentEntry::Key { packet.id(), packet.commodity_type() });
		entry.update(packet);
		recent_entries_view.set_dirty();
	}
}

void ERTAppView::on_show_list() {
	recent_entries_view.hidden(false);
	recent_entries_view.focus();
}

} /* namespace ui */
