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

#include "ui_lcr.hpp"
#include "ui_afsksetup.hpp"
#include "ui_debug.hpp"

#include "baseband_api.hpp"
#include "string_format.hpp"

#include "portapack_persistent_memory.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

namespace ui {

void LCRView::focus() {
	button_setrgsb.focus();
}

LCRView::~LCRView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void LCRView::generate_message() {
	// Modem sync and SOM
	const char lcr_init[8] = { 127, 127, 127, 127, 127, 127, 127, 15 };
	// Eclairage (Auto, Jour, Nuit)
	const char ec_lut[3][2] = { { 'A', 0x00 },
								{ 'J', 0x00 },
								{ 'N', 0x00 } };
	char eom[3] = { 3, 0, 0 };		// EOM and space for checksum
	uint8_t i, pm, bit;
	uint16_t dp;
	uint8_t cp, pp, cur_byte, new_byte;
	
	button_setrgsb.set_text(rgsb);
	
	// Pad litterals to 7 chars (not required ?)
	for (i = 0; i < 5; i++) {
		while (strlen(litteral[i]) < 7) {
			strcat(litteral[i], " ");
		}
	}
	
	// Compose LCR message
	memset(lcr_message, 0, 512);
	memcpy(lcr_message, lcr_init, 8);

	strcat(lcr_message, rgsb);		// Address
	strcat(lcr_message, "PA ");
	
	for (i = 0; i < 5; i++) {
		if (checkboxes[i].value() == true) {
			strcat(lcr_message, "AM=");
			strcat(lcr_message, to_string_dec_uint(i + 1, 1).c_str());
			strcat(lcr_message, " AF=\"");
			strcat(lcr_message, litteral[i]);
			strcat(lcr_message, "\" CL=0 ");
		}
	}
	strcat(lcr_message, "EC=");
	strcat(lcr_message, ec_lut[options_ec.selected_index()]);
	strcat(lcr_message, " SAB=0");
	
	// Checksum
	checksum = 0;
	i = 7;						// Skip modem sync
	while (lcr_message[i]) {
		checksum ^= lcr_message[i];
		i++;
	}
	checksum ^= eom[0];			// EOM char
	checksum &= 0x7F;			// Trim
	eom[1] = checksum;
	
	strcat(lcr_message, eom);
	
	//if (persistent_memory::afsk_config() & 2)
		pm = 0; // Even parity
	//else
	//	pm = 1; // Odd parity

	if (!(persistent_memory::afsk_config() & 8)) {
		// Normal format
		for (dp = 0; dp < strlen(lcr_message); dp++) {
			pp = pm;
			new_byte = 0;
			cur_byte = lcr_message[dp];
			for (cp = 0; cp < 7; cp++) {
				bit = (cur_byte >> cp) & 1;
				pp += bit;
				new_byte |= (bit << (7 - cp));
			}
			lcr_message_data[dp] = new_byte | (pp & 1);
		}
		lcr_message_data[dp++] = 0;
		lcr_message_data[dp] = 0;
	} else {
		// Alt format
		for (dp = 0; dp < strlen(lcr_message); dp++) {
			pp = pm;
			// Do not apply LUT on checksum (last byte) ?
			if (dp != strlen(lcr_message) - 1)
				cur_byte = alt_lookup[(uint8_t)lcr_message[dp] & 0x7F];
			else
				cur_byte = lcr_message[dp];
			for (cp = 0; cp < 8; cp++) {
				if ((cur_byte >> cp) & 1) pp++;
			}
			lcr_message_data[dp * 2] = cur_byte;
			lcr_message_data[(dp * 2) + 1] = 0xFE | (pp & 1);
		}
		lcr_message_data[dp * 2] = 0;
		lcr_message_data[(dp * 2) + 1] = 0;
	}

	//if (persistent_memory::afsk_config() & 1) {
		// LSB first
		// See above
	/*} else {
		// MSB first
		for (dp=0;dp<strlen(lcr_message);dp++) {
			pp = pm;
			cur_byte = lcr_message[dp];
			for (cp=0;cp<7;cp++) {
				if ((cur_byte>>cp)&1) pp++;
			}
			lcr_message_data[dp] = (cur_byte<<1)|(pp&1);
		}
	}*/
}

void LCRView::paint(Painter& painter) {
	uint8_t i;
	char finalstr[24] = {0};
	
	static constexpr Style style_orange {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::orange(),
	};
	
	Point offset = {
		static_cast<Coord>(104),
		static_cast<Coord>(68)
	};
	
	for (i = 0; i < 5; i++) {
		painter.draw_string(
			screen_pos() + offset,
			style_orange,
			litteral[i]
		);
		offset.y += 32;
	}
	
	button_setrgsb.set_text(rgsb);
	
	// Recap: freq @ bps / ALT
	auto fstr = to_string_dec_int(portapack::persistent_memory::tuned_frequency() / 1000, 6);
	auto bstr = to_string_dec_int(portapack::persistent_memory::afsk_bitrate(), 4);
	strcpy(finalstr, fstr.c_str());
	strcat(finalstr, "@");
	strcat(finalstr, bstr.c_str());
	strcat(finalstr, "bps ");
	if (portapack::persistent_memory::afsk_config() & 8)
		strcat(finalstr, "ALT");
	else
		strcat(finalstr, "NRM");
	text_recap.set(finalstr);
}

void LCRView::update_progress() {
	char str[16];
	
	text_status.set("            ");
	
	if (tx_mode == SINGLE) {
		strcpy(str, to_string_dec_uint(repeat_index).c_str());
		strcat(str, "/");
		strcat(str, to_string_dec_uint(portapack::persistent_memory::afsk_repeats()).c_str());
		text_status.set(str);
		progress.set_value(repeat_index);
	} else if (tx_mode == SCAN) {
		strcpy(str, to_string_dec_uint(repeat_index).c_str());
		strcat(str, "/");
		strcat(str, to_string_dec_uint(portapack::persistent_memory::afsk_repeats()).c_str());
		strcat(str, " ");
		strcat(str, to_string_dec_uint(scan_index + 1).c_str());
		strcat(str, "/");
		strcat(str, to_string_dec_uint(LCR_SCAN_COUNT).c_str());
		text_status.set(str);
		progress.set_value(scan_progress);
	} else {
		text_status.set("Ready");
		progress.set_value(0);
	}
}

void LCRView::on_txdone(int n) {
	char str[16];
	
	if (n > 0) {
		// Repeating...
		repeat_index = n + 1;
		if (tx_mode == SCAN) {
			scan_progress++;
			update_progress();
		} else {
			update_progress();
		}
	} else {
		// Done transmitting
		if ((tx_mode == SCAN) && (scan_index < (LCR_SCAN_COUNT - 1))) {
			transmitter_model.disable();
			if (abort_scan) {
				// Kill scan process
				strcpy(str, "Abort @");
				strcat(str, rgsb);
				text_status.set(str);
				progress.set_value(0);
				tx_mode = IDLE;
				abort_scan = false;
				button_scan.set_style(&style_val);
				button_scan.set_text("SCAN");
			} else {
				// Next address
				scan_index++;
				strcpy(rgsb, RGSB_list[scan_index]);
				scan_progress++;
				repeat_index = 1;
				update_progress();
				start_tx(true);
			}
		} else {
			transmitter_model.disable();
			tx_mode = IDLE;
			update_progress();
			button_scan.set_style(&style_val);
			button_scan.set_text("SCAN");
		}
	}	
}

void LCRView::start_tx(const bool scan) {
	bool afsk_alt_format;
	uint8_t afsk_repeats;
	
	afsk_repeats = portapack::persistent_memory::afsk_repeats();
	
	if (scan) {
		if (tx_mode != SCAN) {
			scan_index = 0;
			scan_progress = 1;
			repeat_index = 1;
			tx_mode = SCAN;
			strcpy(rgsb, RGSB_list[0]);
			progress.set_max(LCR_SCAN_COUNT * afsk_repeats);
			update_progress();
		}
	} else {
		tx_mode = SINGLE;
		repeat_index = 1;
		progress.set_max(afsk_repeats);
		update_progress();
	}
	
	generate_message();

	if (portapack::persistent_memory::afsk_config() & 8)
		afsk_alt_format = true;
	else
		afsk_alt_format = false;
	
	transmitter_model.set_tuning_frequency(portapack::persistent_memory::tuned_frequency());
	transmitter_model.set_baseband_configuration({
		.mode = 0,
		.sampling_rate = 2280000U,
		.decimation_factor = 1,
	});
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_lna(40);
	transmitter_model.set_vga(40);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	baseband::set_afsk_data(
		lcr_message_data,
		228000 / portapack::persistent_memory::afsk_bitrate(),
		portapack::persistent_memory::afsk_mark_freq() * (0x40000 * 256) / 2280,
		portapack::persistent_memory::afsk_space_freq() * (0x40000 * 256) / 2280,
		afsk_repeats,
		portapack::persistent_memory::afsk_bw() * 115,		// See proc_fsk_lcr.cpp
		afsk_alt_format
	);
}

void LCRView::on_button_setam(NavigationView& nav, Button& button) {
	textentry(nav, litteral[button.id], 7);
}

LCRView::LCRView(NavigationView& nav) {
	std::string label;
	
	baseband::run_image(portapack::spi_flash::image_tag_afsk);

	memset(litteral, 0, 5 * 8);
	memset(rgsb, 0, 5);
	
	strcpy(rgsb, RGSB_list[0]);
	
	add_children({ {
		&text_recap,
		&options_ec,
		&button_setrgsb,
		&button_txsetup,
		&text_status,
		&progress,
		&button_lcrdebug,
		&button_transmit,
		&button_scan,
		&button_clear
	} });
	
	const auto button_setam_fn = [this, &nav](Button& button) {
		this->on_button_setam(nav, button);
	};
	
	size_t n = 0;
	for(auto& button : buttons) {
		button.on_select = button_setam_fn;
		button.id = n;
		label = "AM " + to_string_dec_uint(n + 1, 1);;
		button.set_text(label);
		button.set_parent_rect({
			static_cast<Coord>(48),
			static_cast<Coord>(n * 32 + 64),
			48, 24
		});
		add_child(&button);
		n++;
	}
	
	n = 0;
	for(auto& checkbox : checkboxes) {
		checkbox.set_parent_rect({
			static_cast<Coord>(16),
			static_cast<Coord>(n * 32 + 64),
			48, 24
		});
		checkbox.set_value(false);
		add_child(&checkbox);
		n++;
	}
	
	n = 0;
	for(auto& rectangle : rectangles) {
		rectangle.set_parent_rect({
			static_cast<Coord>(104 - 2),
			static_cast<Coord>(n * 32 + 68 - 2),
			56 + 4, 16 + 4
		});
		rectangle.set_color(ui::Color::grey());
		rectangle.set_outline(true);
		add_child(&rectangle);
		n++;
	}
	
	button_setrgsb.set_text(rgsb);
	options_ec.set_selected_index(0);	// Auto
	checkboxes[0].set_value(true);
	
	button_transmit.set_style(&style_val);
	button_scan.set_style(&style_val);
	
	button_setrgsb.on_select = [this,&nav](Button&) {
		textentry(nav, rgsb, 4);
	};
	
	button_txsetup.on_select = [&nav](Button&) {
		nav.push<AFSKSetupView>();
	};
	
	button_lcrdebug.on_select = [this, &nav](Button&) {
		generate_message();
		nav.push<DebugLCRView>(std::string(lcr_message), checksum);
	};
	
	button_transmit.on_select = [this](Button&) {
		if (tx_mode == IDLE) start_tx(false);
	};
	
	button_scan.on_select = [this](Button&) {
		if (tx_mode == IDLE)	{
			button_scan.set_style(&style_cancel);
			button_scan.set_text("ABORT");
			start_tx(true);
		} else {
			abort_scan = true;
		}
	};

	button_clear.on_select = [this, &nav](Button&) {
		uint8_t n;
		
		if (tx_mode == IDLE) {
			memset(litteral, 0, 5 * 8);
			options_ec.set_selected_index(0);	// Auto
			for (n = 0; n < 5; n++)
				checkboxes[n].set_value(true);
			set_dirty();
			start_tx(false);
		} else if (tx_mode == SCAN) {
			abort_scan = true;
		}
	};
}

} /* namespace ui */
