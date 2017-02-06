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

#include "audio.hpp"
#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"
using namespace portapack;
using namespace pocsag;

#include "pocsag.hpp"
#include "string_format.hpp"

#include "utility.hpp"

void POCSAGLogger::on_packet(const pocsag::POCSAGPacket& packet, const uint32_t frequency) {
	std::string entry = to_string_dec_uint(frequency) + "Hz " + pocsag::bitrate_str(packet.bitrate()) + " RAW: ";
	
	// Raw hex dump of all the codewords
	for (size_t c = 0; c < 16; c++)
		entry += to_string_hex(packet[c], 8) + " ";
	
	log_file.write_entry(packet.timestamp(), entry);
}

void POCSAGLogger::on_decoded(
	const pocsag::POCSAGPacket& packet,
	const std::string text) {
	
	log_file.write_entry(packet.timestamp(), text);
}

namespace ui {

void POCSAGAppView::update_freq(rf::Frequency f) {
	options_freq.set_selected_index(0);		// "Entered"
	set_target_frequency(f);
	
	portapack::persistent_memory::set_tuned_frequency(f);		// Maybe not ?
	
	auto mhz = to_string_dec_int(f / 1000000, 4);
	auto hz100 = to_string_dec_int((f / 100) % 10000, 4, '0');

	this->button_setfreq.set_text(mhz + "." + hz100);
}

POCSAGAppView::POCSAGAppView(NavigationView& nav) {
	
	baseband::run_image(portapack::spi_flash::image_tag_pocsag);

	add_children({
		&rssi,
		&channel,
		&options_freq,
		&field_rf_amp,
		&field_lna,
		&field_vga,
		&button_setfreq,
		&options_bitrate,
		&check_log,
		&console
	});
	
	receiver_model.set_sampling_rate(3072000);
	receiver_model.set_baseband_bandwidth(1750000);
	receiver_model.enable();
	
	check_log.set_value(logging);
	check_log.on_select = [this](Checkbox&, bool v) {
		logging = v;
	};
	
	options_bitrate.on_change = [this](size_t, OptionsField::value_t v) {
		this->on_bitrate_changed(v);
	};
	options_bitrate.set_selected_index(1);	// 1200bps

	options_freq.on_change = [this](size_t, OptionsField::value_t v) {
		this->on_band_changed(v);
	};
	options_freq.set_by_value(target_frequency());
	
	button_setfreq.on_select = [this,&nav](Button&) {
		auto new_view = nav.push<FrequencyKeypadView>(target_frequency_);
		new_view->on_changed = [this](rf::Frequency f) {
			update_freq(f);
		};
	};

	logger = std::make_unique<POCSAGLogger>();
	if (logger) logger->append("pocsag.txt");
	
	/*const auto new_volume = volume_t::decibel(60 - 99) + audio::headphone::volume_range().max;
	receiver_model.set_headphone_volume(new_volume);
	audio::output::start();*/
}

POCSAGAppView::~POCSAGAppView() {
	//audio::output::stop();
	receiver_model.disable();
	baseband::shutdown();
}

void POCSAGAppView::focus() {
	options_freq.focus();
}

// Useless ?
void POCSAGAppView::set_parent_rect(const Rect new_parent_rect) {
	View::set_parent_rect(new_parent_rect);
}

void POCSAGAppView::on_packet(const POCSAGPacketMessage * message) {
	std::string alphanum_text = "";
	
	// Log raw data whatever it contains
	if (logger && logging)
		logger->on_packet(message->packet, target_frequency());
	
	if (message->packet.flag() != NORMAL) {
		console.writeln(
			"RX ERROR: " + pocsag::flag_str(message->packet.flag()) +
			" Codewords: " + to_string_dec_uint(message->packet[0])
		);
	}
	
	bool result = pocsag_decode_batch(message->packet, &pocsag_state);

	if (result) {
		std::string console_info;
		
		console_info = to_string_time(message->packet.timestamp()) + " ";
		console_info += pocsag::bitrate_str(message->packet.bitrate());
		console_info += " ADDR:" + to_string_dec_uint(pocsag_state.address);
		console_info += " F" + to_string_dec_uint(pocsag_state.function);
		
		if (pocsag_state.out_type == ADDRESS) {
			// Address only
			
			console.writeln(console_info);
			
			if (logger && logging)
				logger->on_decoded(message->packet, console_info);
			
			last_address = pocsag_state.address;
		} else if (pocsag_state.out_type == MESSAGE) {
			if (pocsag_state.address != last_address) {
				// New message
				
				console.writeln(console_info);
				
				console.writeln("Alpha:" + pocsag_state.output);
				
				if (logger && logging) {
					logger->on_decoded(message->packet, console_info);
					logger->on_decoded(message->packet, pocsag_state.output);
				}
				
				last_address = pocsag_state.address;
			} else {
				// Message continues...
				console.writeln(pocsag_state.output);
				
				if (logger && logging)
					logger->on_decoded(message->packet, pocsag_state.output);
			}
		}
	}
}

void POCSAGAppView::on_bitrate_changed(const uint32_t new_bitrate) {
	const pocsag::BitRate bitrates[3] = {
		pocsag::BitRate::FSK512,
		pocsag::BitRate::FSK1200,
		pocsag::BitRate::FSK2400
	};
	
	baseband::set_pocsag(bitrates[new_bitrate]);
}

void POCSAGAppView::on_band_changed(const uint32_t new_band_frequency) {
	if (new_band_frequency) set_target_frequency(new_band_frequency);
}

void POCSAGAppView::set_target_frequency(const uint32_t new_value) {
	target_frequency_ = new_value;
	receiver_model.set_tuning_frequency(new_value);
}

uint32_t POCSAGAppView::target_frequency() const {
	return target_frequency_;
}

} /* namespace ui */
