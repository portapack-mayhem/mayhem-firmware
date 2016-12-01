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

#include "ui_adsbtx.hpp"
#include "ui_alphanum.hpp"

#include "string_format.hpp"
#include "portapack.hpp"
#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

namespace ui {

void ADSBTxView::focus() {
	button_transmit.focus();
}

ADSBTxView::~ADSBTxView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void ADSBTxView::paint(Painter& painter) {
	button_callsign.set_text(callsign);
}

void ADSBTxView::generate_frame_pos() {
	uint8_t b, c, s, time_parity, bitn;
	char ch;
	std::string str_debug;
	uint16_t altitude_coded = (38000 + 1000) / 25;
	uint32_t LAT, LON;
	float delta_lat, yz, rlat, delta_lon, xz;
	uint8_t adsb_crc[14];			// Temp buffer
	uint32_t crc_poly = 0x1205FFF;
	
	LAT = 0;
	
	adsb_frame[0] = (options_format.selected_index_value() << 3) | 5;	// DF and CA
	adsb_frame[1] = 0x48;	// ICAO24
	adsb_frame[2] = 0x40;
	adsb_frame[3] = 0xD6;
	adsb_frame[4] = 0x58;	// TC, SS and NICsb
	
	// altitude = (feet + 1000) / 25
	
	// LAT:
	// index j = floor(59*latcprE-60*latcprO+0.50)
	// latE = DlatE*(mod(j,60)+latcprE)
	// latO = DlatO*(mod(j,59)+latcprO)
	// if latE >= 270 -> latE -= 360
	// if latO >= 270 -> latO -= 360
	//time_parity = 0;	// 0~1
	//delta_lat = 90.0 / (60.0 - (time_parity / 4.0));
	//yz = 524288.0 * (mod(lat, delta_lat) / delta_lat);	// Round to int !
	//rlat = delta_lat * ((yz / 524288.0) + int(lat / delta_lat));
	//delta_lon = 360.0 / (NL(rlat) - time_parity);
	//xz = 524288.0 * (mod(lon, delta_lon) / delta_lon);	// Round to int !
	/*if (time_parity) {
		A = sign(rlat0)[NL(rlat0) - NL(rlat1)];
	}*/
	// int xz and yz, then:
	// xz >>= 2;
	// yz >>= 2;
	// To get 17 bits
	
	// aaaaaaa Q bbbb
	adsb_frame[5] = ((altitude_coded & 0x7F0) >> 3) | 1;
	adsb_frame[6] = ((altitude_coded & 0x00F) << 4) | (LAT >> 15);		// Then 0, even/odd, and the 2 LAT-CPR MSBs
	
	adsb_frame[11] = 0x00;	// Clear CRC
	adsb_frame[12] = 0x00;
	adsb_frame[13] = 0x00;
	
	// Compute CRC
	memcpy(adsb_crc, adsb_frame, 14);
	for (c = 0; c < 11; c++) {
		for (b = 0; b < 8; b++) {
			if ((adsb_crc[c] << b) & 0x80) {
				for (s = 0; s < 25; s++) {
					bitn = (c * 8) + b + s;
					if ((crc_poly >> s) & 1) adsb_crc[bitn >> 3] ^= (0x80 >> (bitn & 7));
				}
			}
		}
	}
	// Insert CRC in frame
	for (c = 0; c < 3; c++)
		adsb_frame[c + 11] = adsb_crc[c + 11];
	
	// Convert to binary
	for (c = 0; c < 112; c++)
		adsb_bin[c] = (adsb_frame[c >> 3] >> (7 - (c & 7))) & 1;
	
	// Display for debug
	str_debug = "";
	for (c = 0; c < 7; c++)
		str_debug += to_string_hex(adsb_frame[c], 2);
	text_frame_a.set(str_debug);
	str_debug = "";
	for (c = 0; c < 7; c++)
		str_debug += to_string_hex(adsb_frame[c + 7], 2);
	text_frame_b.set(str_debug);
}

void ADSBTxView::generate_frame_id() {
	uint8_t b, c, s, bitn;
	char ch;
	std::string str_debug;
	std::string callsign_formatted(8, '_');
	uint64_t callsign_coded = 0;
	uint8_t adsb_crc[14];			// Temp buffer
	uint32_t crc_poly = 0x1205FFF;
	
	// Init frame
	//memset(adsb_frame, 0, 120);
	
	adsb_frame[0] = (options_format.selected_index_value() << 3) | 5;	// DF and CA
	adsb_frame[1] = 0x48;	// ICAO24
	adsb_frame[2] = 0x40;
	adsb_frame[3] = 0xD6;
	adsb_frame[4] = 0x20;	// TC
	
	adsb_frame[11] = 0x00;	// Clear CRC
	adsb_frame[12] = 0x00;
	adsb_frame[13] = 0x00;
	
	// Translate and code callsign
	for (c = 0; c < 8; c++) {
		ch = callsign[c];
		for (s = 0; s < 64; s++) {
			if (ch == icao_id_lut[s]) break;
		}
		if (s < 64) {
			ch = icao_id_lut[s];
		} else {
			ch = ' ';
			s = 32;
		}
		callsign_coded |= ((uint64_t)s << ((7 - c) * 6));
		callsign_formatted[c] = ch;
	}
	
	// Insert callsign in frame
	for (c = 0; c < 6; c++)
		adsb_frame[c + 5] = (callsign_coded >> ((5 - c) * 8)) & 0xFF;
	
	// Compute CRC
	memcpy(adsb_crc, adsb_frame, 14);
	for (c = 0; c < 11; c++) {
		for (b = 0; b < 8; b++) {
			if ((adsb_crc[c] << b) & 0x80) {
				for (s = 0; s < 25; s++) {
					bitn = (c * 8) + b + s;
					if ((crc_poly >> s) & 1) adsb_crc[bitn >> 3] ^= (0x80 >> (bitn & 7));
				}
			}
		}
	}
	// Insert CRC in frame
	for (c = 0; c < 3; c++)
		adsb_frame[c + 11] = adsb_crc[c + 11];
	
	// Convert to binary
	for (c = 0; c < 112; c++)
		adsb_bin[c] = (adsb_frame[c >> 3] >> (7 - (c & 7))) & 1;
	
	// Display for debug
	str_debug = "";
	for (c = 0; c < 7; c++)
		str_debug += to_string_hex(adsb_frame[c], 2);
	text_frame_a.set(str_debug);
	str_debug = "";
	for (c = 0; c < 7; c++)
		str_debug += to_string_hex(adsb_frame[c + 7], 2);
	text_frame_b.set(str_debug);

	text_message.set(callsign_formatted);
}

void ADSBTxView::start_tx() {
	transmitter_model.set_tuning_frequency(452000000);		// FOR TESTING - DEBUG
	transmitter_model.set_baseband_configuration({
		.mode = 0,
		.sampling_rate = 2000000U,		// Good ?
		.decimation_factor = 1,
	});
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_lna(40);
	transmitter_model.set_vga(40);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	memcpy(shared_memory.tx_data, adsb_bin, 112);
	baseband::set_adsb();
}

void ADSBTxView::on_txdone(const int n) {
	size_t sr;

	if (n == 200) {
		transmitter_model.disable();
		progress.set_value(0);
		
		tx_mode = IDLE;
		button_transmit.set_style(&style_val);
		button_transmit.set_text("START");
	} else {
		progress.set_value(n);
	}
}

ADSBTxView::ADSBTxView(NavigationView& nav) {
	(void)nav;
	
	baseband::run_image(portapack::spi_flash::image_tag_adsb_tx);

	// http://openflights.org

	add_children({ {
		&text_format,
		&options_format,
		&text_icaolabel,
		&button_icao,
		&text_callsign,
		&button_callsign,
		&text_frame_a,
		&text_frame_b,
		&progress,
		&text_message,
		&button_transmit
	} });
	
	options_format.set_by_value(17);	// Mode S
	
	progress.set_max(122);
	
	options_format.on_change = [this](size_t i, int32_t v) {
		(void)i;
		(void)v;
		generate_frame_id();
	};
	button_callsign.on_select = [this, &nav](Button&) {
		textentry(nav, callsign, 9);
	};
	
	button_transmit.set_style(&style_val);
	
	generate_frame_id();

	// Single transmit
	button_transmit.on_select = [this, &nav](Button&) {
		if (tx_mode == IDLE) {
			tx_mode = SINGLE;
			button_transmit.set_style(&style_cancel);
			button_transmit.set_text("Wait");
			generate_frame_id();
			start_tx();
		}
	};
}

} /* namespace ui */
