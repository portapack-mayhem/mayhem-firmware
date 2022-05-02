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

static bool use_kpa = true;

std::string type(Reading::Type type) {
	return to_string_dec_uint(toUType(type), 2);
}

std::string id(TransponderID id) {
	return to_string_hex(id.value(), 8);
}

std::string pressure(Pressure pressure) {
	if(use_kpa){
		return to_string_dec_int(pressure.kilopascal(), 3);
	}
	return to_string_dec_int(pressure.psi(), 3);
	
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

template<>
void RecentEntriesTable<TPMSRecentEntries>::draw(
	const Entry& entry,
	const Rect& target_rect,
	Painter& painter,
	const Style& style
) {
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
	painter.draw_string(target_rect.location(), style, line);
}

TPMSAppView::TPMSAppView(NavigationView&) {
	baseband::run_image(portapack::spi_flash::image_tag_tpms);

	add_children({
		&rssi,
		&channel,
		&options_band,
		&field_rf_amp,
		&field_lna,
		&field_vga,
		&options_type,
	});

	// load app settings
	auto rc = settings.load("rx_tpms", &app_settings);
	if(rc == SETTINGS_OK) {
		field_lna.set_value(app_settings.lna);
		field_vga.set_value(app_settings.vga);
		field_rf_amp.set_value(app_settings.rx_amp);
		options_band.set_by_value(app_settings.rx_frequency);
	}
	else options_band.set_by_value(receiver_model.tuning_frequency());

	radio::enable({
		tuning_frequency(),
		sampling_rate,
		baseband_bandwidth,
		rf::Direction::Receive,
		receiver_model.rf_amp(),
		static_cast<int8_t>(receiver_model.lna()),
		static_cast<int8_t>(receiver_model.vga()),
	});

	options_band.on_change = [this](size_t, OptionsField::value_t v) {
		this->on_band_changed(v);
	};
	options_band.set_by_value(target_frequency());

	options_type.on_change = [this](size_t, int32_t i) {		
		if (i == 0){
			tpms::format::use_kpa = true;
		} else if (i == 1){
			tpms::format::use_kpa = false;
		}	
		update_type();
	};

	options_type.set_selected_index(0, true);

	logger = std::make_unique<TPMSLogger>();
	if( logger ) {
		logger->append(u"tpms.txt");
	}
}

TPMSAppView::~TPMSAppView() {


	// save app settings
	app_settings.rx_frequency = target_frequency_;
	settings.save("rx_tpms", &app_settings);

	radio::disable();

	baseband::shutdown();
}

void TPMSAppView::focus() {
	options_band.focus();
}

void TPMSAppView::update_type() {
	if (tpms::format::use_kpa){
		remove_child(&recent_entries_view_psi);
		add_child(&recent_entries_view_kpa);
		recent_entries_view_kpa.set_parent_rect(view_normal_rect);
	} else {
		remove_child(&recent_entries_view_kpa);
		add_child(&recent_entries_view_psi);
		recent_entries_view_psi.set_parent_rect(view_normal_rect);
	}	
}

void TPMSAppView::set_parent_rect(const Rect new_parent_rect) {
	View::set_parent_rect(new_parent_rect);

	view_normal_rect  = { 0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height };

	update_type();
}

void TPMSAppView::on_packet(const tpms::Packet& packet) {
	if( logger ) {
		logger->on_packet(packet, target_frequency());
	}

	const auto reading_opt = packet.reading();
	if( reading_opt.is_valid() ) {
		const auto reading = reading_opt.value();
		auto& entry = ::on_packet(recent, TPMSRecentEntry::Key { reading.type(), reading.id() });
		entry.update(reading);
		
		if(tpms::format::use_kpa){
			recent_entries_view_kpa.set_dirty();
		} else {
			recent_entries_view_psi.set_dirty();
		}
	}
}

void TPMSAppView::on_show_list() {
	if(tpms::format::use_kpa){
		recent_entries_view_kpa.hidden(false);
		recent_entries_view_kpa.focus();
	} else {
		recent_entries_view_psi.hidden(false);
		recent_entries_view_psi.focus();
	}
}

void TPMSAppView::on_band_changed(const uint32_t new_band_frequency) {
	set_target_frequency(new_band_frequency);
}

void TPMSAppView::set_target_frequency(const uint32_t new_value) {
	target_frequency_ = new_value;
	radio::set_tuning_frequency(tuning_frequency());
}

uint32_t TPMSAppView::target_frequency() const {
	return target_frequency_;
}

uint32_t TPMSAppView::tuning_frequency() const {
	return target_frequency() - (sampling_rate / 4);
}

} /* namespace ui */
