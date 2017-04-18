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

#include "afsk.hpp"
#include "lcr.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"

#include "portapack_persistent_memory.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;
using namespace afsk;

namespace ui {

void LCRView::focus() {
	button_setrgsb.focus();
}

LCRView::~LCRView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void LCRView::paint(Painter& painter) {
	uint8_t i;
	std::string final_str;
	
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
		offset += { 0, 32 };
	}
	
	button_setrgsb.set_text(rgsb);
	
	// Recap: freq @ bps
	final_str = to_string_dec_int(portapack::persistent_memory::tuned_frequency() / 1000, 6);
	final_str += '@';
	final_str += to_string_dec_int(portapack::persistent_memory::afsk_bitrate(), 4);
	final_str += "bps ";
	final_str += afsk_formats[portapack::persistent_memory::afsk_format()].shortname;
	text_recap.set(final_str);
}

std::vector<std::string> LCRView::parse_litterals() {
	std::vector<std::string> litterals;
	
	for (size_t i = 0; i < 5; i++) {
		if (checkboxes[i].value())
			litterals.push_back(litteral[i]);
	}
	
	return litterals;
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
		strcat(str, to_string_dec_uint(scan_count).c_str());
		text_status.set(str);
		progress.set_value(scan_progress);
	} else {
		text_status.set("Ready");
		progress.set_value(0);
	}
}

void LCRView::on_txdone(int n) {
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
		if ((tx_mode == SCAN) && (scan_index < (scan_count - 1))) {
			transmitter_model.disable();
			// Next address
			scan_index++;
			strcpy(rgsb, &scan_list[options_scanlist.selected_index()].addresses[scan_index * 5]);
			scan_progress++;
			repeat_index = 1;
			update_progress();
			start_tx(true);
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
	uint8_t afsk_format;
	uint8_t afsk_repeats;
	
	afsk_repeats = portapack::persistent_memory::afsk_repeats();
	
	if (scan) {
		if (tx_mode != SCAN) {
			scan_index = 0;
			scan_count = scan_list[options_scanlist.selected_index()].count;
			scan_progress = 1;
			repeat_index = 1;
			tx_mode = SCAN;
			strcpy(rgsb, &scan_list[options_scanlist.selected_index()].addresses[0]);
			progress.set_max(scan_count * afsk_repeats);
			update_progress();
		}
	} else {
		tx_mode = SINGLE;
		repeat_index = 1;
		progress.set_max(afsk_repeats);
		update_progress();
	}
	
	button_setrgsb.set_text(rgsb);
	afsk::generate_data(lcr::generate_message(rgsb, parse_litterals(), options_ec.selected_index()), lcr_message_data);

	switch (portapack::persistent_memory::afsk_format()) {
		case 0:
		case 1:
		case 2:
			afsk_format = 0;
			break;
		
		case 3:
			afsk_format = 1;
			break;
		
		default:
			afsk_format = 0;
	}

	transmitter_model.set_tuning_frequency(portapack::persistent_memory::tuned_frequency());
	transmitter_model.set_sampling_rate(1536000U);
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_lna(40);
	transmitter_model.set_vga(40);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();

	memcpy(shared_memory.bb_data.data, lcr_message_data, 300);
	
	baseband::set_afsk_data(
		(153600 * 5) / portapack::persistent_memory::afsk_bitrate(),
		portapack::persistent_memory::afsk_mark_freq() * 437 * 5, 	// (0x40000 * 256) / (153600 / 25),
		portapack::persistent_memory::afsk_space_freq() * 437 * 5, 	// (0x40000 * 256) / (153600 / 25),
		afsk_repeats,
		portapack::persistent_memory::afsk_bw() * 115,				// See proc_afsk.cpp
		afsk_format
	);
}

void LCRView::on_button_setam(NavigationView& nav, Button& button) {
	textentry(nav, litteral[button.id], 7);
}

LCRView::LCRView(NavigationView& nav) {
	std::string label;
	
	baseband::run_image(portapack::spi_flash::image_tag_afsk);
	
	strcpy(rgsb, &scan_list[0].addresses[0]);
	
	add_children({
		&text_recap,
		&options_ec,
		&button_setrgsb,
		&button_txsetup,
		&text_status,
		&progress,
		&button_lcrdebug,
		&button_transmit,
		&text_scanlist,
		&options_scanlist,
		&button_scan,
		&button_clear
	});
	
	options_scanlist.set_selected_index(0);
	
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
		nav.push<DebugLCRView>(lcr::generate_message(rgsb, parse_litterals(), options_ec.selected_index()));
	};
	
	button_transmit.on_select = [this](Button&) {
		if (tx_mode == IDLE) start_tx(false);
	};
	
	button_scan.on_select = [this](Button&) {
		char str[16];
		
		if (tx_mode == IDLE)	{
			button_scan.set_style(&style_cancel);
			button_scan.set_text("ABORT");
			start_tx(true);
		} else {
			// Kill scan process
			baseband::kill_afsk();
			strcpy(str, "Abort @");
			strcat(str, rgsb);
			text_status.set(str);
			progress.set_value(0);
			tx_mode = IDLE;
			button_scan.set_style(&style_val);
			button_scan.set_text("SCAN");
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
		}
	};
}

} /* namespace ui */
