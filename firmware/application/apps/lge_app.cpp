/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2019 Furrtek
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

// The UI for this app is in French because it concerns leisure centers
// only established in France. "LGE" stands for a trademark I'd rather
// not spell out completely here.

#include "lge_app.hpp"

#include "baseband_api.hpp"
#include "ui_textentry.hpp"

#include "string_format.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

namespace ui {

void LGEView::focus() {
	options_trame.focus();
}

LGEView::~LGEView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void LGEView::generate_lge_frame(const uint8_t command, const uint16_t address_a, const uint16_t address_b, std::vector<uint8_t>& data) {
	std::array<uint8_t, 5> header = {
		command,
		(uint8_t)(address_a & 255),
		(uint8_t)(address_a >> 8),
		(uint8_t)(address_b & 255),
		(uint8_t)(address_b >> 8),
	};
	
	data.insert(data.begin(), header.begin(), header.end());
	
	frame_size = rfm69.gen_frame(data);
	
	for (auto b : data)
		console.write(to_string_hex(b, 2) + " ");
}

void LGEView::generate_frame_touche() {
	// 0001.89s
	// 0D 96 02 12 0E 00 46 28 01 45 27 01 44 23 66 30
	std::vector<uint8_t> data { 0x46, 0x28, 0x01, 0x45, 0x27, 0x01, 0x44, 0x23 };
	
	console.write("\n\x1B\x07Touche:\x1B\x10");
	generate_lge_frame(0x96, (field_joueur.value() << 8) | field_salle.value(), 0x0001, data);
}

void LGEView::generate_frame_pseudo() {
	// 0040.48s:
	// 30 02 1A 00 19 00 FF 00 02 19 42 52 45 42 49 53 20 00 00 00 00 00 00 00 00 00
	// 04 01 B0 04 7F 1F 11 33 40 1F 22 01 07 00 00 01 07 00 00 63 05 00 00 99 A2 
	
	std::vector<uint8_t> data { };
	std::array<uint8_t, 3> data_header = { 0xFF, 0x00, 0x02 };
	std::array<uint8_t, 22> data_footer = {
		0x01, 0xB0, 0x04, 0x7F,
		0x1F, 0x11, 0x33, 0x40,
		0x1F, 0x22, 0x01, 0x07,
		0x00, 0x00, 0x01, 0x07,
		0x00, 0x00, 0x63, 0x05,
		0x00, 0x00
	};
	uint32_t c;
	
	//data_header[2] = field_salle.value();	// ?
	//data_footer[0] = field_salle.value();	// ?
	
	data.insert(data.begin(), data_header.begin(), data_header.end());
	
	data.push_back(field_joueur.value());
	
	c = 0;
	for (auto &ch : pseudo) {
		data.push_back(ch);
		c++;
	}
	// Space at the end, is this required ?
	data.push_back(0x20);
	// Pad with zeroes
	while (++c < 16)
		data.push_back(0x00);
	
	data.push_back(field_equipe.value());
	
	data.insert(data.end(), data_footer.begin(), data_footer.end());
	
	console.write("\n\x1B\x0ESet pseudo:\x1B\x10");
	
	generate_lge_frame(0x02, 0x001A, field_joueur.value(), data);
}

void LGEView::generate_frame_equipe() {
	// 0041.83s:
	// 3D 03 FF FF FF FF 02 03 01 52 4F 55 47 45 00 00 00 00 00 00 00 00 00 00 00 00
	// 02 56 45 52 54 45 00 00 00 00 00 00 00 00 00 00 00 01 03 42 4C 45 55 45 00 00
	// 00 00 00 00 00 00 00 00 00 02 43 29

	std::vector<uint8_t> data { };
	std::array<uint8_t, 2> data_header = { 0x02, 0x01 };
	uint32_t c;

	data.insert(data.begin(), data_header.begin(), data_header.end());
	
	data.push_back(field_equipe.value());
	
	c = 0;
	for (auto &ch : pseudo) {
		data.push_back(ch);
		c++;
	}
	// Pad with zeroes
	while (c++ < 16)
		data.push_back(0x00);
	
	data.push_back(field_equipe.value() - 1);	// Color ?
	
	console.write("\n\x1B\x0ASet equipe:\x1B\x10");
	
	generate_lge_frame(0x03, data);
}

void LGEView::generate_frame_broadcast_pseudo() {
	// 0043.86s:
	// 3D 04 FF FF FF FF 02 03 19 42 52 45 42 49 53 20 00 00 00 00 00 00 00 00 00 04
	// 07 50 4F 4E 45 59 20 00 00 00 00 00 00 00 00 00 00 05 1B 41 42 42 59 20 00 00
	// 00 00 00 00 00 00 00 00 00 04 0A 02

	std::vector<uint8_t> data { };
	std::array<uint8_t, 2> data_header = { 0x02, 0x01 };
	uint32_t c;

	data.insert(data.begin(), data_header.begin(), data_header.end());
	
	data.push_back(field_joueur.value());
	
	c = 0;
	for (auto &ch : pseudo) {
		data.push_back(ch);
		c++;
	}
	// Space at the end, is this required ?
	data.push_back(0x20);
	// Pad with zeroes
	while (++c < 16)
		data.push_back(0x00);
	
	data.push_back(field_equipe.value());
	
	console.write("\n\x1B\x09" "Broadcast pseudo:\x1B\x10");
	
	generate_lge_frame(0x04, data);
}

void LGEView::generate_frame_start() {
	// 0166.13s:
	// 0A 05 FF FF FF FF 02 EC FF FF FF A3 35
	std::vector<uint8_t> data { 0x02, 0xEC, 0xFF, 0xFF, 0xFF };
	
	//data[0] = field_salle.value();	// ?
	
	console.write("\n\x1B\x0DStart:\x1B\x10");
	generate_lge_frame(0x05, data);
}

void LGEView::generate_frame_gameover() {
	std::vector<uint8_t> data { (uint8_t)field_salle.value() };
	
	console.write("\n\x1B\x0CGameover:\x1B\x10");
	generate_lge_frame(0x0D, data);
}

void LGEView::generate_frame_collier() {
	uint8_t flags = 0;
	
	// Custom
	// 0C 00 13 37 13 37 id flags channel playerid zapduty zaptime checksum CRC CRC
	// channel: field_channel
	// playerid: field_joueur
	// zapduty: field_power
	// zaptime: field_duration
	
	if (checkbox_heartbeat.value())
		flags |= 1;
	if (checkbox_rxtick.value())
		flags |= 2;
	
	uint8_t checksum = 0;
	uint8_t id = (uint8_t)field_id.value();
	
	std::vector<uint8_t> data {
		id,
		flags,
		(uint8_t)field_salle.value(),
		(uint8_t)field_joueur.value(),
		(uint8_t)field_power.value(),
		(uint8_t)(field_duration.value() * 10)
	};
	
	for (auto &v : data)
		checksum += v;
	
	data.push_back(checksum - id);
	
	console.write("\n\x1B\x06" "Config:\x1B\x10");
	generate_lge_frame(0x00, 0x3713, 0x3713, data);
}

void LGEView::start_tx() {
	if (tx_mode == ALL) {
		transmitter_model.set_tuning_frequency(channels[channel_index]);
		tx_view.on_show();	// Refresh tuning frequency display
		tx_view.set_dirty();
	}
	transmitter_model.set_sampling_rate(2280000);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();

	chThdSleep(100);
		
	baseband::set_fsk_data(frame_size * 8, 2280000 / 9600, 4000, 256);
}

void LGEView::stop_tx() {
	tx_mode = IDLE;
	transmitter_model.disable();
	tx_view.set_transmitting(false);
}

void LGEView::on_tx_progress(const uint32_t progress, const bool done) {
	(void)progress;
	
	if (!done) return;
	
	transmitter_model.disable();
	
	/*if (repeats < 2) {
		chThdSleep(100);
		repeats++;
		start_tx();
	} else {*/
		if (tx_mode == ALL) {
			if (channel_index < 2) {
				channel_index++;
				repeats = 0;
				start_tx();
			} else {
				stop_tx();
			}
		} else {
			stop_tx();
		}
	//}
}

LGEView::LGEView(NavigationView& nav) {
	baseband::run_image(portapack::spi_flash::image_tag_fsktx);
	
	add_children({
		&labels,
		&options_trame,
		&field_salle,
		&button_texte,
		&field_equipe,
		&field_joueur,
		&field_id,
		&field_power,
		&field_duration,
		&checkbox_heartbeat,
		&checkbox_rxtick,
		&checkbox_channels,
		&console,
		&tx_view
	});
	
	field_salle.set_value(1);
	field_equipe.set_value(1);
	field_joueur.set_value(1);
	field_id.set_value(1);
	field_power.set_value(1);
	field_duration.set_value(2);
	
	button_texte.on_select = [this, &nav](Button&) {
		text_prompt(
			nav,
			pseudo,
			15,
			[this](std::string& buffer) {
				button_texte.set_text(buffer);
			});
	};
	
	tx_view.on_edit_frequency = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			receiver_model.set_tuning_frequency(f);
		};
	};
	
	tx_view.on_start = [this]() {
		if (tx_mode == IDLE) {
			auto i = options_trame.selected_index_value();
			if (i == 0)
				generate_frame_touche();
			else if (i == 1)
				generate_frame_pseudo();
			else if (i == 2)
				generate_frame_equipe();
			else if (i == 3)
				generate_frame_broadcast_pseudo();
			else if (i == 4)
				generate_frame_start();
			else if (i == 5)
				generate_frame_gameover();
			else if (i == 6)
				generate_frame_collier();
			
			repeats = 0;
			channel_index = 0;
			tx_mode = checkbox_channels.value() ? ALL : SINGLE;
			tx_view.set_transmitting(true);
			start_tx();
		}
	};
	
	tx_view.on_stop = [this]() {
		stop_tx();
	};
}

} /* namespace ui */
