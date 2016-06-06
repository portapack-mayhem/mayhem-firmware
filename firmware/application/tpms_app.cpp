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

#include "baseband_api.hpp"

#include "portapack.hpp"
using namespace portapack;

#include "string_format.hpp"

#include "utility.hpp"

namespace tpms {

namespace format {

std::string type(Reading::Type type) {
	return to_string_dec_uint(toUType(type), 2);
}

std::string id(TransponderID id) {
	return to_string_hex(id.value(), 8);
}

std::string pressure(Pressure pressure) {
	return to_string_dec_int(pressure.kilopascal(), 3);
}

std::string temperature(Temperature temperature) {
	return to_string_dec_int(temperature.celsius(), 3);
}

std::string flags(Flags flags) {
	return to_string_hex(flags, 2);
}

static std::string signal_type(SignalType signal_type) {
	switch(signal_type) {
	case SignalType::FSK_19k2_Schrader:		return "FSK 38400 19200 Schrader";
	case SignalType::OOK_8k192_Schrader:	return "OOK - 8192 Schrader";
	case SignalType::OOK_8k4_Schrader:		return "OOK - 8400 Schrader";
	default:								return "- - - -";
	}
}

} /* namespace format */

} /* namespace tpms */

void TPMSLogger::on_packet(const tpms::Packet& packet, const uint32_t target_frequency) {
	const auto hex_formatted = packet.symbols_formatted();

	// TODO: function doesn't take uint64_t, so when >= 1<<32, weirdness will ensue!
	const auto tuning_frequency_str = to_string_dec_uint(target_frequency, 10);

	std::string entry = tuning_frequency_str + " " + tpms::format::signal_type(packet.signal_type()) + " " + hex_formatted.data + "/" + hex_formatted.errors;
	log_file.write_entry(packet.received_at(), entry);
}

const TPMSRecentEntry::Key TPMSRecentEntry::invalid_key = { tpms::Reading::Type::None, 0 };

void TPMSRecentEntry::update(const tpms::Reading& reading) {
	received_count++;

	if( reading.pressure().is_valid() ) {
		last_pressure = reading.pressure();
	}
	if( reading.temperature().is_valid() ) {
		last_temperature = reading.temperature();
	}
	if( reading.flags().is_valid() ) {
		last_flags = reading.flags();
	}
}

namespace ui {

static const std::array<std::pair<std::string, size_t>, 6> tpms_columns { {
	{ "Tp", 2 },
	{ "ID", 8 },
	{ "kPa", 3 },
	{ "C", 3 },
	{ "Cnt", 3 },
	{ "Fl", 2 },
} };

template<>
void RecentEntriesView<TPMSRecentEntries>::draw_header(
	const Rect& target_rect,
	Painter& painter,
	const Style& style
) {
	auto x = 0;
	for(const auto& column : tpms_columns) {
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
void RecentEntriesView<TPMSRecentEntries>::draw(
	const Entry& entry,
	const Rect& target_rect,
	Painter& painter,
	const Style& style,
	const bool is_selected
) {
	const auto& draw_style = is_selected ? style.invert() : style;

	std::string line = tpms::format::type(entry.type) + " " + tpms::format::id(entry.id);

	if( entry.last_pressure.is_valid() ) {
		line += " " + tpms::format::pressure(entry.last_pressure.value());
	} else {
		line += " " "   ";
	}

	if( entry.last_temperature.is_valid() ) {
		line += " " + tpms::format::temperature(entry.last_temperature.value());
	} else {
		line += " " "   ";
	}

	if( entry.received_count > 999 ) {
		line += " +++";
	} else {
		line += " " + to_string_dec_uint(entry.received_count, 3);
	}

	if( entry.last_flags.is_valid() ) {
		line += " " + tpms::format::flags(entry.last_flags.value());
	} else {
		line += " " "  ";
	}

	line.resize(target_rect.width() / 8, ' ');
	painter.draw_string(target_rect.pos, draw_style, line);
}

TPMSAppView::TPMSAppView(NavigationView&) {
	add_children({ {
		&rssi,
		&channel,
		&field_lna,
		&field_vga,
		&recent_entries_view,
	} });

	radio::enable({
		tuning_frequency(),
		sampling_rate,
		baseband_bandwidth,
		rf::Direction::Receive,
		false,
		static_cast<int8_t>(receiver_model.lna()),
		static_cast<int8_t>(receiver_model.vga()),
		1,
	});

	baseband::start({
		.mode = 5,
		.sampling_rate = sampling_rate,
		.decimation_factor = 1,
	});

	logger = std::make_unique<TPMSLogger>();
	if( logger ) {
		logger->append("tpms.txt");
	}
}

TPMSAppView::~TPMSAppView() {
	baseband::stop();
	radio::disable();
}

void TPMSAppView::focus() {
	recent_entries_view.focus();
}

void TPMSAppView::set_parent_rect(const Rect new_parent_rect) {
	View::set_parent_rect(new_parent_rect);
	recent_entries_view.set_parent_rect({ 0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height });
}

void TPMSAppView::on_packet(const tpms::Packet& packet) {
	if( logger ) {
		logger->on_packet(packet, target_frequency());
	}

	const auto reading_opt = packet.reading();
	if( reading_opt.is_valid() ) {
		const auto reading = reading_opt.value();
		recent.on_packet({ reading.type(), reading.id() }, reading);
		recent_entries_view.set_dirty();
	}
}

void TPMSAppView::on_show_list() {
	recent_entries_view.hidden(false);
	recent_entries_view.focus();
}

uint32_t TPMSAppView::target_frequency() const {
	return initial_target_frequency;
}

uint32_t TPMSAppView::tuning_frequency() const {
	return target_frequency() - (sampling_rate / 4);
}

} /* namespace ui */
