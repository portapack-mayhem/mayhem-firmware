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
	options_enctype.focus();
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
			debug_text += encoder_def->bit_format[symfield_word.get_sym(i++)];
	}
	
	draw_waveform();
}

void EncodersView::draw_waveform() {
	uint32_t n, length;

	length = debug_text.length();
	
	for (n = 0; n < length; n++) {
		if (debug_text[n] == '0')
			waveform_buffer[n] = 0;
		else
			waveform_buffer[n] = 1;
	}
	
	waveform.set_length(length);
	waveform.set_dirty();
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

void EncodersView::on_txdone(int n, const bool txdone) {
	//char str[16];
	
	if (!txdone) {
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
			tx_view.set_transmitting(false);
		//}
	}
}

void EncodersView::on_tuning_frequency_changed(rf::Frequency f) {
	transmitter_model.set_tuning_frequency(f);
}

void EncodersView::start_tx(const bool scan) {
	char ook_bitstream[256];
	uint32_t ook_bitstream_length;
	(void)scan;
	
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
	memset(ook_bitstream, 0, 256);
	
	size_t n = 0;
	for (auto c : debug_text) {
		if (c != '0')
			ook_bitstream[n >> 3] |= (1 << (7 - (n & 7)));
		n++;
	}
	
	ook_bitstream_length = n;

	transmitter_model.set_sampling_rate(2280000U);
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_lna(40);
	transmitter_model.set_vga(40);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	memcpy(shared_memory.bb_data.data, ook_bitstream, 256);
	
	baseband::set_ook_data(
		ook_bitstream_length,
		// 2280000/2 = 1140000Hz = 0,877192982us
		// numberfield_clk.value() / encoder_def->clk_per_fragment
		// 455000 / 12 = 37917Hz = 26,37339452us
		228000 / ((numberfield_clk.value() * 1000) / encoder_def->clk_per_fragment),
		encoder_def->repeat_min,
		encoder_def->pause_symbols
	);
}

void EncodersView::on_type_change(size_t index) {
	std::string word_format, format_string = "";
	size_t word_length;
	char symbol_type;
	//size_t address_length;
	
	enc_type = index;

	encoder_def = &encoder_defs[enc_type];

	numberfield_clk.set_value(encoder_def->default_speed / 1000);
	
	// SymField setup
	word_length = encoder_def->word_length;
	symfield_word.set_length(word_length);
	size_t n = 0, i = 0;
	while (n < word_length) {
		symbol_type = encoder_def->word_format.at(i++);
		if (symbol_type == 'A') {
			symfield_word.set_symbol_list(n++, encoder_def->address_symbols);
			format_string += 'A';
		} else if (symbol_type == 'D') {
			symfield_word.set_symbol_list(n++, encoder_def->data_symbols);
			format_string += 'D';
		}
	}
	
	// Ugly :( Pad to erase
	while (format_string.length() < 24)
		format_string += ' ';
	
	text_format.set(format_string);

	generate_frame();
}

void EncodersView::on_show() {
	// TODO: Remove ?
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
	
	// Default encoder def
	encoder_def = &encoder_defs[0];

	add_children({
		&labels,
		&options_enctype,
		&numberfield_clk,
		&numberfield_bitduration,
		&numberfield_wordduration,
		//&field_debug,
		&symfield_word,
		&text_format,
		//&text_format_a,	// DEBUG
		//&text_format_d,	// DEBUG
		&waveform,
		&text_status,
		&progress,
		&tx_view
	});
	
	// Load encoder types
	for (i = 0; i < ENC_TYPES_COUNT; i++)
		enc_options.emplace_back(std::make_pair(encoder_defs[i].name, i));
	
	options_enctype.on_change = [this](size_t index, int32_t value) {
		(void)value;
		this->on_type_change(index);
	};
	
	options_enctype.set_options(enc_options);
	options_enctype.set_selected_index(0);
	
	symfield_word.on_change = [this]() {
		this->generate_frame();
	};
	
	// DEBUG
	/*field_debug.on_change = [this](int32_t value) {
		uint32_t l;
		de_bruijn debruijn_seq;
		debruijn_seq.init(value);
		l = 1;
		l <<= value;
		l--;
		if (l > 25)
			l = 25;
		text_format.set(to_string_bin(debruijn_seq.compute(l), 25));
	};*/
	
	// Selecting input clock changes symbol and word duration
	numberfield_clk.on_change = [this](int32_t value) {
		//int32_t new_value = 1000000 / (((float)value * 1000) / encoder_def->clk_per_symbol);
		// value is in kHz, new_value is in us
		int32_t new_value = 1000000 / ((value * 1000) / encoder_def->clk_per_symbol);
		if (new_value != numberfield_bitduration.value()) {
			numberfield_bitduration.set_value(new_value, false);
			numberfield_wordduration.set_value(new_value * encoder_def->word_length, false);
		}
	};
	
	// Selecting symbol duration changes input clock and word duration
	numberfield_bitduration.on_change = [this](int32_t value) {
		int32_t new_value = 1000000 / (((float)value * 1000) / encoder_def->clk_per_symbol);
		if (new_value != numberfield_clk.value()) {
			numberfield_clk.set_value(new_value, false);
			numberfield_wordduration.set_value(value * encoder_def->word_length, false);
		}
	};
	
	// Selecting word duration changes input clock and symbol duration
	numberfield_wordduration.on_change = [this](int32_t value) {
		int32_t new_value = value / encoder_def->word_length;
		if (new_value != numberfield_bitduration.value()) {
			numberfield_bitduration.set_value(new_value, false);
			numberfield_clk.set_value(1000000 / (((float)new_value * 1000) / encoder_def->clk_per_symbol), false);
		}
	};
	
	tx_view.on_edit_frequency = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			receiver_model.set_tuning_frequency(f);
		};
	};
	
	tx_view.on_start = [this]() {
		tx_view.set_transmitting(true);
		start_tx(false);
	};
	
	tx_view.on_stop = [this]() {
		tx_view.set_transmitting(false);
	};
}

} /* namespace ui */
