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

#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"
using namespace portapack;

#include "string_format.hpp"

#include "utility.hpp"

#define POCSAG_SYNC     0x7CD215D8
#define POCSAG_IDLE		0x7A89C197

namespace pocsag {

namespace format {

static std::string bitrate_str(BitRate bitrate) {
	switch (bitrate) {
		case BitRate::FSK512:	return "512 ";
		case BitRate::FSK1200:	return "1200";
		case BitRate::FSK2400:	return "2400";
		default:				return "????";
	}
}

static std::string flag_str(PacketFlag packetflag) {
	switch (packetflag) {
		case PacketFlag::NORMAL:	return "OK";
		case PacketFlag::TIMED_OUT:	return "TIMED OUT";
		case PacketFlag::TOO_LONG:	return "TOO LONG";
		default:					return "";
	}
}

} /* namespace format */

} /* namespace pocsag */

void POCSAGLogger::on_packet(const pocsag::POCSAGPacket& packet, const uint32_t frequency) {
	std::string entry = pocsag::format::bitrate_str(packet.bitrate()) + " " + to_string_dec_uint(frequency) + "Hz ";
	
	// Raw hex dump of all the codewords
	for (size_t c = 0; c < 16; c++)
		entry += to_string_hex(packet[c], 8) + " ";
	
	log_file.write_entry(packet.timestamp(), entry);
}

void POCSAGLogger::on_decoded(
	const pocsag::POCSAGPacket& packet,
	const std::string info,
	const std::string text) {
	
	// Decoded address and message
	log_file.write_entry(packet.timestamp(), info);
	log_file.write_entry(packet.timestamp(), text);
}

namespace ui {

void POCSAGAppView::update_freq(rf::Frequency f) {
	char finalstr[10] = { 0 };
	
	options_freq.set_selected_index(0);
	set_target_frequency(f);
	
	portapack::persistent_memory::set_tuned_frequency(f);		// Maybe not ?
	
	auto mhz = to_string_dec_int(f / 1000000, 4);
	auto hz100 = to_string_dec_int((f / 100) % 10000, 4, '0');

	strcat(finalstr, mhz.c_str());
	strcat(finalstr, ".");
	strcat(finalstr, hz100.c_str());

	this->button_setfreq.set_text(finalstr);
}

POCSAGAppView::POCSAGAppView(NavigationView& nav) {
	baseband::run_image(portapack::spi_flash::image_tag_pocsag);

	add_children({ {
		&rssi,
		&channel,
		&options_freq,
		&button_setfreq,
		&field_rf_amp,
		&field_lna,
		&field_vga,
		&console
	} });

	radio::enable({
		tuning_frequency(),
		sampling_rate,
		baseband_bandwidth,
		rf::Direction::Receive,
		receiver_model.rf_amp(),
		static_cast<int8_t>(receiver_model.lna()),
		static_cast<int8_t>(receiver_model.vga()),
		1,
	});

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
	
	baseband::set_pocsag();
}

POCSAGAppView::~POCSAGAppView() {
	radio::disable();
	baseband::shutdown();
}

void POCSAGAppView::focus() {
	options_freq.focus();
}

void POCSAGAppView::set_parent_rect(const Rect new_parent_rect) {
	View::set_parent_rect(new_parent_rect);
}

void POCSAGAppView::on_packet(const POCSAGPacketMessage * message) {
	bool eom = false;
	uint32_t codeword;
	std::string alphanum_text = "";
	char ascii_char;
	
	for (size_t i = 0; i < 16; i++) {
		codeword = message->packet[i];
		
		if (codeword & 0x80000000U) {
			// Message
			ascii_data |= ((codeword >> 11) & 0xFFFFF);
			ascii_idx += 20;
			
			// Packet 20 bits to 7 bit reversed ASCII
			while (ascii_idx >= 7) {
				ascii_idx -= 7;
				ascii_char = (ascii_data >> ascii_idx) & 0x7F;
				
				// Bottom's up
				ascii_char = (ascii_char & 0xF0) >> 4 | (ascii_char & 0x0F) << 4;	// 01234567 -> 45670123
				ascii_char = (ascii_char & 0xCC) >> 2 | (ascii_char & 0x33) << 2;	// 45670123 -> 67452301
				ascii_char = (ascii_char & 0xAA) >> 2 | (ascii_char & 0x55);		// 67452301 -> *7654321
   
				// Translate non-printable chars
				if ((ascii_char < 32) || (ascii_char > 126))
					output_text += "[" + to_string_dec_uint(ascii_char) + "]";
				else
					output_text += ascii_char;

			}
			ascii_data = ascii_data << 20;
		} else {
			// Address
			if ((codeword == POCSAG_IDLE) || (codeword == POCSAG_SYNC)) {
				eom = true;
			} else {
				if ((codeword) && (!address)) {
					// Set address if none set and codeword is valid
					function = (codeword >> 11) & 3;
					address = ((codeword >> 10) & 0x1FFFF8) | ((codeword >> 1) & 7);
				}
			}
		}
	}
	
	if (logger) logger->on_packet(message->packet, target_frequency());

	if (eom) {
		std::string console_info;
		
		console_info = to_string_time(message->packet.timestamp()) + " ";
		
		if (address || function) {
			console_info += pocsag::format::bitrate_str( message->packet.bitrate()) + " ";
			console_info += pocsag::format::flag_str(message->packet.flag()) + " ";
			console_info += "ADDR:" + to_string_dec_uint(address) + " F:" + to_string_dec_uint(function);
			
			console.writeln(console_info);
			
			if (output_text != "") console.writeln("Alpha:" + output_text);
			
			if (logger) logger->on_decoded(message->packet, console_info, output_text);
			
		} else {
			console_info += pocsag::format::bitrate_str(message->packet.bitrate()) + " Tone only";
			
			console.writeln(console_info);
		}

		output_text = "";
		ascii_idx = 0;
		ascii_data = 0;
		batch_cnt = 0;
		address = 0;
		function = 0;
	} else {
		batch_cnt++;
	}
}

void POCSAGAppView::on_band_changed(const uint32_t new_band_frequency) {
	if (new_band_frequency) set_target_frequency(new_band_frequency);
}

void POCSAGAppView::set_target_frequency(const uint32_t new_value) {
	target_frequency_ = new_value;
	radio::set_tuning_frequency(tuning_frequency());
}

uint32_t POCSAGAppView::target_frequency() const {
	return target_frequency_;
}

uint32_t POCSAGAppView::tuning_frequency() const {
	return target_frequency() - (sampling_rate / 4);
}

} /* namespace ui */
