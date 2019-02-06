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

#include "lge_app.hpp"

#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"

#include "crc.hpp"
#include "string_format.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

namespace ui {

void LGEView::focus() {
	tx_view.focus();
}

LGEView::~LGEView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void LGEView::generate_frame() {
	CRC<16> crc { 0x1021, 0x90BE };
	std::vector<uint8_t> frame { };
	uint8_t payload[9] = { 0x06, 0x0D, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x00 };
	uint8_t out = 0;
	uint32_t c;
	
	payload[6] = field_zone.value();	// Zone
	
	// Preamble
	// Really is 0xAA but the RFM69 skips the very last bit (bug ?)
	// so the whole preamble should be shifted right 1 bit to simulate that
	for (c = 0; c < 5; c++)
		frame.push_back(0x55);
	
	frame.push_back(0x2D);	// Sync word
	frame.push_back(0xD4);
	
	crc.process_bytes(payload, 9 - 2);
	
	payload[7] = crc.checksum() >> 8;
	payload[8] = crc.checksum() & 0xFF;
	
	// Manchester-encode payload
	for (c = 0; c < 9; c++) {
		uint8_t byte = payload[c];
		for (uint32_t b = 0; b < 8; b++) {
			out <<= 2;
			
			if (byte & 0x80)
				out |= 0b10;
			else
				out |= 0b01;
			
			if ((b & 3) == 3)
				frame.push_back(out);
			
			byte <<= 1;
		}
	}
	
	frame_size = frame.size();
	
	/*std::string debug_str { "" };
	
	for (c = 0; c < 10; c++)
		debug_str += (to_string_hex(frame[c], 2) + " ");
	
	text_messagea.set(debug_str);

	debug_str = "";
	for (c = 15; c < frame_size; c++)
		debug_str += (to_string_hex(frame[c], 2) + " ");
	
	text_messageb.set(debug_str);*/

	// Copy for baseband
	memcpy(shared_memory.bb_data.data, frame.data(), frame_size);
}

void LGEView::start_tx() {
	if (tx_mode == ALL) {
		transmitter_model.set_tuning_frequency(channels[channel_index]);
		tx_view.on_show();	// Refresh tuning frequency display
		tx_view.set_dirty();
	}
	transmitter_model.set_sampling_rate(2280000);
	transmitter_model.set_rf_amp(true);
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
	
	if (repeats < 2) {
		chThdSleep(100);
		repeats++;
		start_tx();
	} else {
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
	}
}

LGEView::LGEView(NavigationView& nav) {
	baseband::run_image(portapack::spi_flash::image_tag_fsktx);
	
	add_children({
		&labels,
		&field_zone,
		&checkbox_channels,
		&text_messagea,
		&text_messageb,
		&tx_view
	});
	
	field_zone.set_value(1);
	checkbox_channels.set_value(true);
	
	generate_frame();
	
	field_zone.on_change = [this](int32_t) {
		generate_frame();
	};
	
	tx_view.on_edit_frequency = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			receiver_model.set_tuning_frequency(f);
		};
	};
	
	tx_view.on_start = [this]() {
		if (tx_mode == IDLE) {
			generate_frame();
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
