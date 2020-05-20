/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "pocsag_app.hpp"

#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;
using namespace pocsag;

#include "string_format.hpp"
#include "utility.hpp"

void POCSAGLogger::log_raw_data(const pocsag::POCSAGPacket& packet, const uint32_t frequency) {
	std::string entry = "Raw: F:" + to_string_dec_uint(frequency) + "Hz " +
						pocsag::bitrate_str(packet.bitrate()) + " Codewords:";
	
	// Raw hex dump of all the codewords
	for (size_t c = 0; c < 16; c++)
		entry += to_string_hex(packet[c], 8) + " ";
	
	log_file.write_entry(packet.timestamp(), entry);
}

void POCSAGLogger::log_decoded(
	const pocsag::POCSAGPacket& packet,
	const std::string text) {
	
	log_file.write_entry(packet.timestamp(), text);
}

namespace ui {

void POCSAGAppView::update_freq(rf::Frequency f) {
	set_target_frequency(f);
	portapack::persistent_memory::set_tuned_frequency(f);	// Maybe not ?
}

POCSAGAppView::POCSAGAppView(NavigationView& nav) {
	uint32_t ignore_address;
	
	baseband::run_image(portapack::spi_flash::image_tag_pocsag);

	add_children({
		&rssi,
		&channel,
		&field_rf_amp,
		&field_lna,
		&field_vga,
		&field_frequency,
		&options_bitrate,
		&options_phase,
		&check_log,
		&check_ignore,
		&sym_ignore,
		&console
	});
	
	receiver_model.set_sampling_rate(3072000);
	receiver_model.set_baseband_bandwidth(1750000);
	receiver_model.enable();
	
	field_frequency.set_value(receiver_model.tuning_frequency());
	field_frequency.set_step(receiver_model.frequency_step());
	field_frequency.on_change = [this](rf::Frequency f) {
		update_freq(f);
	};
	field_frequency.on_edit = [this, &nav]() {
		// TODO: Provide separate modal method/scheme?
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			update_freq(f);
			field_frequency.set_value(f);
		};
	};
	
	check_log.set_value(logging);
	check_log.on_select = [this](Checkbox&, bool v) {
		logging = v;
	};
	
	options_bitrate.on_change = [this](size_t, OptionsField::value_t v) {
		on_config_changed(v, options_phase.selected_index_value());
	};
	options_bitrate.set_selected_index(1);	// 1200bps
	
	options_phase.on_change = [this](size_t, OptionsField::value_t v) {
		on_config_changed(options_bitrate.selected_index_value(),v);
	};
	check_ignore.set_value(ignore);
	check_ignore.on_select = [this](Checkbox&, bool v) {
		ignore = v;
	};
	
	ignore_address = persistent_memory::pocsag_ignore_address();
	for (size_t c = 0; c < 7; c++) {
		sym_ignore.set_sym(6 - c, ignore_address % 10);
		ignore_address /= 10;
	}

	logger = std::make_unique<POCSAGLogger>();
	if (logger)
		logger->append("pocsag.txt");
}

POCSAGAppView::~POCSAGAppView() {	
	// Save ignored address
	persistent_memory::set_pocsag_ignore_address(sym_ignore.value_dec_u32());
	
	receiver_model.disable();
	baseband::shutdown();
}

void POCSAGAppView::focus() {
	field_frequency.focus();
}

// Useless ?
void POCSAGAppView::set_parent_rect(const Rect new_parent_rect) {
	View::set_parent_rect(new_parent_rect);
}

void POCSAGAppView::on_packet(const POCSAGPacketMessage * message) {
	std::string alphanum_text = "";
	
	if (message->packet.flag() != NORMAL)
		console.writeln("\n\x1B\x0CRC ERROR: " + pocsag::flag_str(message->packet.flag()));
	else {
		pocsag_decode_batch(message->packet, &pocsag_state);

		if ((ignore) && (pocsag_state.address == sym_ignore.value_dec_u32())) {
			// Ignore (inform, but no log)
			//console.write("\n\x1B\x03" + to_string_time(message->packet.timestamp()) +
			//			" Ignored address " + to_string_dec_uint(pocsag_state.address));
			return;
		}

		std::string console_info;
		
		console_info = "\n" + to_string_datetime(message->packet.timestamp(), HM);
		console_info += " " + pocsag::bitrate_str(message->packet.bitrate());
		console_info += " ADDR:" + to_string_dec_uint(pocsag_state.address);
		console_info += " F" + to_string_dec_uint(pocsag_state.function);

		// Store last received address for POCSAG TX
		persistent_memory::set_pocsag_last_address(pocsag_state.address);
		
		if (pocsag_state.out_type == ADDRESS) {
			// Address only
			
			console.write(console_info);
			
			if (logger && logging) {
				logger->log_decoded(message->packet, to_string_dec_uint(pocsag_state.address) +
													" F" + to_string_dec_uint(pocsag_state.function) +
													" Address only");
			}
			
			last_address = pocsag_state.address;
		} else if (pocsag_state.out_type == MESSAGE) {
			if (pocsag_state.address != last_address) {
				// New message
				console.writeln(console_info);
				console.write(pocsag_state.output);
				
				last_address = pocsag_state.address;
			} else {
				// Message continues...
				console.write(pocsag_state.output);
			}
			
			if (logger && logging)
				logger->log_decoded(message->packet, to_string_dec_uint(pocsag_state.address) +
													" F" + to_string_dec_uint(pocsag_state.function) +
													" Alpha: " + pocsag_state.output);
		}
	}
	
	// Log raw data whatever it contains
	if (logger && logging)
		logger->log_raw_data(message->packet, target_frequency());
}

void POCSAGAppView::on_config_changed(const uint32_t new_bitrate, bool new_phase) {
	baseband::set_pocsag(pocsag_bitrates[new_bitrate], new_phase);
}

void POCSAGAppView::set_target_frequency(const uint32_t new_value) {
	target_frequency_ = new_value;
	receiver_model.set_tuning_frequency(new_value);
}

uint32_t POCSAGAppView::target_frequency() const {
	return target_frequency_;
}

} /* namespace ui */
