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

#include "ui_encoders.hpp"

#include "baseband_api.hpp"
#include "string_format.hpp"

#include "portapack_persistent_memory.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

namespace ui {

void EncodersView::focus() {
	bitfields[0].focus();
}

EncodersView::~EncodersView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void EncodersView::generate_frame() {
	uint8_t i;
	
	debug_text.clear();
	i = 0;
	for (auto c : encoder_def->format) {
		if (c <= 'J')
			debug_text += encoder_def->bit_format.at(bitfields[i++].value());
	}
	
	//text_status.set(debug_text.substr(0, 28));
	
	if (visible()) parent()->set_dirty();
	
	//afsk::generate_data(lcr_message, lcr_message_data);
}

void EncodersView::paint(Painter& painter) {
	Coord x = 8, x_inc;
	Coord y, prev_y = 0;
	
	painter.fill_rectangle( { 0, 144, 240, 24 }, Color::black() );
	
	x_inc = 240 / debug_text.length();
	
	for (auto c : debug_text) {
		if (c == '0')
			y = 23;
		else
			y = 0;
		
		if (prev_y != y) painter.draw_rectangle( { x, 144, 1, 24 }, Color::yellow() );
		painter.draw_rectangle( { x, 144 + y, x_inc, 1 }, Color::yellow() );
		
		prev_y = y;
		x += x_inc;
		if (x >= (240 - x_inc)) break;
	}
}

void EncodersView::update_progress() {
	/*char str[16];
	
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
	}*/
}

void EncodersView::on_txdone(int n) {
	/*char str[16];
	
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
				strcpy(rgsb, &scan_list[options_scanlist.selected_index()].addresses[scan_index * 5]);
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
	}*/
}

void EncodersView::start_tx(const bool scan) {
	/*if (scan) {
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
	}*/
	
	generate_frame();

	/*
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
		portapack::persistent_memory::afsk_mark_freq() * (0x40000 * 256) / (228000 / 25),
		portapack::persistent_memory::afsk_space_freq() * (0x40000 * 256) / (228000 / 25),
		afsk_repeats,
		portapack::persistent_memory::afsk_bw() * 115,		// See proc_fsk_lcr.cpp
		afsk_format
	);*/
}

void EncodersView::on_bitfield() {
	generate_frame();
}

void EncodersView::on_type_change(size_t index) {
	enc_type = index;
	
	encoder_def = &encoder_defs[enc_type];
	
	numberfield_clk.set_value(encoder_def->default_frequency / 1000);
	
	size_t n = 0;
	for (auto& bitfield : bitfields) {
		if (n < encoder_def->word_length) {
			bitfield.hidden(false);
			bitfield.set_range(0, encoder_def->bit_states - 1);
		} else {
			bitfield.hidden(true);
		}
		n++;
	}
	
	generate_frame();
}

void EncodersView::on_show() {
	options_enctype.set_selected_index(enc_type);
	on_type_change(enc_type);
}

EncodersView::EncodersView(NavigationView& nav) {
	//baseband::run_image(portapack::spi_flash::image_tag_ook);

	add_children({ {
		&text_enctype,
		&options_enctype,
		&text_bitfield,
		&text_clk,
		&numberfield_clk,
		&text_kHz,
		&text_bitduration,
		&numberfield_bitduration,
		&text_us1,
		&text_wordduration,
		&numberfield_wordduration,
		&text_us2,
		&text_waveform,
		&text_status,
		&progress,
		&button_transmit
	} });
	
	options_enctype.on_change = [this](size_t index, int32_t value) {
		(void)value;
		this->on_type_change(index);
	};
	
	numberfield_clk.on_change = [this](int32_t value) {
		int32_t new_value = 1000000 / (((float)value * 1000) / encoder_def->clk_per_bit);
		if (new_value != numberfield_bitduration.value()) {
			numberfield_bitduration.set_value(new_value, false);
			numberfield_wordduration.set_value(new_value * encoder_def->word_length, false);
		}
	};
	
	numberfield_bitduration.on_change = [this](int32_t value) {
		int32_t new_value = 1000000 / (((float)value * 1000) / encoder_def->clk_per_bit);
		if (new_value != numberfield_clk.value()) {
			numberfield_clk.set_value(new_value, false);
			numberfield_wordduration.set_value(value * encoder_def->word_length, false);
		}
	};
	
	numberfield_wordduration.on_change = [this](int32_t value) {
		int32_t new_value = value / encoder_def->word_length;
		if (new_value != numberfield_bitduration.value()) {
			numberfield_bitduration.set_value(new_value, false);
			numberfield_clk.set_value(1000000 / (((float)new_value * 1000) / encoder_def->clk_per_bit), false);
		}
	};

	const auto bitfield_fn = [this](int32_t value) {
		(void)value;
		this->on_bitfield();
	};
	
	size_t n = 0;
	for (auto& bitfield : bitfields) {
		bitfield.on_change = bitfield_fn;
		bitfield.id = n;
		bitfield.set_value(0);
		bitfield.set_parent_rect({
			static_cast<Coord>(16 + 8 * n),
			static_cast<Coord>(80),
			8, 16
		});
		add_child(&bitfield);
		n++;
	}
	
	options_enctype.set_selected_index(0);
	
	button_transmit.set_style(&style_val);

	button_transmit.on_select = [this](Button&) {
		if (tx_mode == IDLE) start_tx(false);
	};
}

} /* namespace ui */
