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
#include <math.h>

using namespace portapack;

namespace ui {

void EncodersView::focus() {
	bitfield.focus();
}

EncodersView::~EncodersView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void EncodersView::generate_frame() {
	size_t i;
	
	debug_text.clear();
	
	i = 0;
	for (auto c : encoder_def->word_format) {
		if (c == 'S')
			debug_text += encoder_def->sync;
		else
			debug_text += encoder_def->bit_format.at(bitfield.value(i));
		i++;
	}
	
	draw_waveform();
}

void EncodersView::draw_waveform() {
	float x = 0, x_inc;
	Coord y, prev_y = 1;
	uint8_t prelude_length = 0; 	//encoder_def->sync.length();
	
	// Clear
	painter_->fill_rectangle( { 0, 160, 240, 24 }, Color::black() );
	
	x_inc = 230.0 / (debug_text.length() - prelude_length);
	
	for (auto c : debug_text) {		//.substr(prelude_length)
		if (c == '0')
			y = 23;
		else
			y = 0;
		
		// Edge
		if (prev_y != y) painter_->draw_rectangle( { (Coord)x, 160, 1, 24 }, Color::yellow() );
		// Level
		painter_->draw_rectangle( { (Coord)x, 160 + y, ceil(x_inc), 1 }, Color::yellow() );
		
		prev_y = y;
		x += x_inc;
	}
}

void EncodersView::paint(Painter& painter) {
	painter_ = &painter;
	draw_waveform();
}

void EncodersView::update_progress() {
	char str[16];
	
	// text_status.set("            ");
	
	if (tx_mode == SINGLE) {
		strcpy(str, to_string_dec_uint(repeat_index).c_str());
		strcat(str, "/");
		strcat(str, to_string_dec_uint(encoder_def->repeat_min).c_str());
		text_status.set(str);
		progress.set_value(repeat_index);
	/*} else if (tx_mode == SCAN) {
		strcpy(str, to_string_dec_uint(repeat_index).c_str());
		strcat(str, "/");
		strcat(str, to_string_dec_uint(portapack::persistent_memory::afsk_repeats()).c_str());
		strcat(str, " ");
		strcat(str, to_string_dec_uint(scan_index + 1).c_str());
		strcat(str, "/");
		strcat(str, to_string_dec_uint(scan_count).c_str());
		text_status.set(str);
		progress.set_value(scan_progress);*/
	} else {
		text_status.set("Ready");
		progress.set_value(0);
	}
}

void EncodersView::on_txdone(int n) {
	//char str[16];
	
	if (n > 0) {
		// Repeating...
		repeat_index = n + 1;
		/*if (tx_mode == SCAN) {
			scan_progress++;
			update_progress();
		} else {*/
			update_progress();
		//}
	} else {
		// Done transmitting
		/*if ((tx_mode == SCAN) && (scan_index < (scan_count - 1))) {
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
		} else {*/
			transmitter_model.disable();
			tx_mode = IDLE;
			text_status.set("Done");
			progress.set_value(0);
		//}
	}
}

void EncodersView::start_tx(const bool scan) {
	char ook_bitstream[64];
	uint32_t ook_bitstream_length;
	
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
	} else {*/
		tx_mode = SINGLE;
		repeat_index = 1;
		progress.set_max(encoder_def->repeat_min);
		update_progress();
	//}
	
	generate_frame();
	
	// Clear bitstream
	memset(ook_bitstream, 0, 64);
	
	size_t n = 0;
	for (auto c : debug_text) {
		if (c != '0')
			ook_bitstream[n >> 3] |= (1 << (7 - (n & 7)));
		n++;
	}
	
	ook_bitstream_length = n; // - 1;

	transmitter_model.set_tuning_frequency(433920000);		// TODO: Make modifiable !
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
	
	memcpy(shared_memory.tx_data, ook_bitstream, 64);
	
	baseband::set_ook_data(
		ook_bitstream_length,
		// 2280000/2 = 1140000Hz = 0,877192982us
		// numberfield_clk.value() / encoder_def->clk_per_symbol
		// 455000 / 12 = 37917Hz = 26,37339452us
		228000 / ((numberfield_clk.value() * 1000) / encoder_def->clk_per_symbol),
		encoder_def->repeat_min,
		encoder_def->pause_symbols
	);
}

void EncodersView::on_type_change(size_t index) {
	std::string word_format;
	//size_t data_length;
	//size_t address_length;
	
	enc_type = index;

	encoder_def = &encoder_defs[enc_type];

	numberfield_clk.set_value(encoder_def->default_frequency / 1000);
	
	bitfield.set_length(encoder_def->word_length);
	bitfield.set_range(0, encoder_def->address_bit_states - 1);
	
	/*word_format = encoder_def->word_format;
	size_t address_start = word_format.find_first_of("A");
	size_t data_start = word_format.find_first_of("D");	
	size_t format_length = word_format.length();
	
	if (address_start == std::string::npos) address_start = format_length;
	if (data_start == std::string::npos) data_start = format_length;
	
	// Never did anything so dirty :(
	if (!address_start) {
		address_length = data_start;
		data_length = format_length - address_length;
	} else {
		data_length = address_start;
		address_length = format_length - data_length;
	}
	
	if (address_length) {
		text_format_a.hidden(false);
		text_format_a.set_parent_rect(
			{ (2 + address_start) * 8, 	12 * 8, 	address_length * 8, 	16 }
		);
		text_format_a.set_style(&style_address);
		text_format_a.set(std::string(address_length, 'A'));
	} else {
		text_format_a.hidden(true);
	}
	
	if (data_length) {
		text_format_d.hidden(false);
		text_format_d.set_parent_rect(
			{ (2 + data_start) * 8, 	12 * 8, 	data_length * 8, 		16 }
		);
		text_format_d.set_style(&style_data);
		text_format_d.set(std::string(data_length, 'D'));
	} else {
		text_format_d.hidden(true);
	}*/

	generate_frame();
}

void EncodersView::on_show() {
	options_enctype.set_selected_index(enc_type);
	on_type_change(enc_type);
}

EncodersView::EncodersView(NavigationView& nav) {
	using name_t = std::string;
	using value_t = int32_t;
	using option_t = std::pair<name_t, value_t>;
	using options_t = std::vector<option_t>;
	options_t enc_options;
	size_t i;
	
	baseband::run_image(portapack::spi_flash::image_tag_ook);
	
	encoder_def = &encoder_defs[0];

	add_children({ {
		&text_enctype,
		&options_enctype,
		&text_clk,
		&numberfield_clk,
		&text_kHz,
		&text_bitduration,
		&numberfield_bitduration,
		&text_us1,
		&text_wordduration,
		&numberfield_wordduration,
		&text_us2,
		&text_bitfield,
		&bitfield,
		//&text_format_a,	// DEBUG
		//&text_format_d,	// DEBUG
		&text_waveform,
		&text_status,
		&progress,
		&button_transmit
	} });
	
	for (i = 0; i < ENC_TYPES_COUNT; i++)
		enc_options.emplace_back(std::make_pair(encoder_defs[i].name, i));
	
	options_enctype.set_options(enc_options);
	options_enctype.set_selected_index(0);
	
	bitfield.on_change = [this]() {
		this->generate_frame();
	};
	
	button_transmit.set_style(&style_val);
	
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

	button_transmit.on_select = [this, &nav](Button&) {
		if (tx_mode == IDLE) start_tx(false);
	};
}

} /* namespace ui */
