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
#include "cpld_update.hpp"

using namespace portapack;

namespace ui {

EncodersConfigView::EncodersConfigView(
	NavigationView&, Rect parent_rect
) {
	using option_t = std::pair<std::string, int32_t>;
	std::vector<option_t> enc_options;
	size_t i;

	set_parent_rect(parent_rect);
	hidden(true);

	// Default encoder def
	encoder_def = &encoder_defs[0];

	add_children({
		&labels,
		&options_enctype,
		&field_repeat_min,
		&field_clk,
		&field_clk_step,
		&field_frameduration,
		&field_frameduration_step,
		&symfield_word,
		&text_format,
		&waveform
	});

	// Load encoder types in option field
	for (i = 0; i < ENC_TYPES_COUNT; i++)
		enc_options.emplace_back(std::make_pair(encoder_defs[i].name, i));

	options_enctype.on_change = [this](size_t index, int32_t) {
		on_type_change(index);
	};

	options_enctype.set_options(enc_options);
	options_enctype.set_selected_index(0);

	symfield_word.on_change = [this]() {
		generate_frame();
	};

	// Selecting input clock changes symbol and word duration
	field_clk.on_change = [this](int32_t value) {
		// value is in kHz, new_value is in us
		int32_t new_value = (encoder_def->clk_per_symbol * 1000000) / (value * 1000);
		if (new_value != field_frameduration.value())
			field_frameduration.set_value(new_value * encoder_def->word_length, false);
	};

	field_clk_step.on_change = [this](size_t, int32_t value) {
		field_clk.set_step(value);
	};

	// Selecting word duration changes input clock and symbol duration
	field_frameduration.on_change = [this](int32_t value) {
		// value is in us, new_value is in kHz
		int32_t new_value = (value * 1000) / (encoder_def->word_length * encoder_def->clk_per_symbol);
		if (new_value != field_clk.value())
			field_clk.set_value(1000000 / new_value, false);
	};

	field_frameduration_step.on_change = [this](size_t, int32_t value) {
		field_frameduration.set_step(value);
	};
}

void EncodersConfigView::focus() {
	options_enctype.focus();
}

void EncodersConfigView::on_type_change(size_t index) {
	std::string format_string = "";
	size_t word_length;
	char symbol_type;

	encoder_def = &encoder_defs[index];

	field_clk.set_value(encoder_def->default_speed / 1000);
	field_repeat_min.set_value(encoder_def->repeat_min);

	// SymField setup
	word_length = encoder_def->word_length;
	symfield_word.set_length(word_length);
	size_t n = 0, i = 0;
	while (n < word_length) {
		symbol_type = encoder_def->word_format[i++];
		if (symbol_type == 'A') {
			symfield_word.set_symbol_list(n++, encoder_def->address_symbols);
			format_string += 'A';
		} else if (symbol_type == 'D') {
			symfield_word.set_symbol_list(n++, encoder_def->data_symbols);
			format_string += 'D';
		}
	}

	// Ugly :( Pad to erase
	format_string.append(24 - format_string.size(), ' ');

	text_format.set(format_string);

	generate_frame();
}

void EncodersConfigView::on_show() {
	options_enctype.set_selected_index(0);
	on_type_change(0);
}

void EncodersConfigView::draw_waveform() {
	size_t length = frame_fragments.length();

	for (size_t n = 0; n < length; n++)
		waveform_buffer[n] = (frame_fragments[n] == '0') ? 0 : 1;

	waveform.set_length(length);
	waveform.set_dirty();
}

void EncodersConfigView::generate_frame() {
	size_t i = 0;

	frame_fragments.clear();

	for (auto c : encoder_def->word_format) {
		if (c == 'S')
			frame_fragments += encoder_def->sync;
		else
			frame_fragments += encoder_def->bit_format[symfield_word.get_sym(i++)];
	}

	draw_waveform();
}

uint8_t EncodersConfigView::repeat_min() {
	return field_repeat_min.value();
}

uint32_t EncodersConfigView::samples_per_bit() {
	return OOK_SAMPLERATE / ((field_clk.value() * 1000) / encoder_def->clk_per_fragment);
}

uint32_t EncodersConfigView::pause_symbols() {
	return encoder_def->pause_symbols;
}

void EncodersScanView::focus() {
	field_length.focus();
}

unsigned int EncodersScanView::scan_sequence(const int n, uint8_t *dest) {
	const int k = 2;
	int l = 1;
	int idx = 0;
	uint8_t v[n];

	memset(v, 0, sizeof(v));

	// Duval's algorithm for generating de Bruijn sequence
	while (1) {
		if (n % l == 0) {
			for (int i = 0; i < l; i++) {
				dest[idx >> 3] |= v[i] << (7 - (idx % 8));
				idx++;
			}
		}

		for (int i = l; i < n; i++)
			v[i] = v[i - l];

		for (l = n; l > 0 && v[l - 1] >= k - 1; l--) ;

		if (l == 0)
			break;

		v[l - 1]++;
	}

	return idx;
}

EncodersScanView::EncodersScanView(
	NavigationView&, Rect parent_rect
) {
	set_parent_rect(parent_rect);
	hidden(true);

	add_children({
		&labels,
		&field_length
	});

	field_length.set_value(8);
}

void EncodersView::focus() {
	tab_view.focus();
}

EncodersView::~EncodersView() {
	// save app settings
	app_settings.tx_frequency = transmitter_model.tuning_frequency();
	settings.save("tx_ook", &app_settings);

	transmitter_model.disable();
	hackrf::cpld::load_sram_no_verify(); // ghost signal c/m to the problem at the exit .
	baseband::shutdown(); // better this function after load_sram()
}

void EncodersView::update_progress() {
	std::string str_buffer;

	if (tx_mode == SINGLE || tx_mode == SCAN) {
		str_buffer = to_string_dec_uint(repeat_index) + "/" + to_string_dec_uint(repeat_min);
		text_status.set(str_buffer);
		progressbar.set_value(repeat_index);
	} else {
		text_status.set("Ready");
		progressbar.set_value(0);
	}
}

void EncodersView::on_tx_progress(const uint32_t progress, const bool done) {
	if (!done) {
		// Repeating...
		repeat_index = progress + 1;
		update_progress();
	} else {
		// Done transmitting
		transmitter_model.disable();
		tx_mode = IDLE;
		text_status.set("Done");
		progressbar.set_value(0);
		tx_view.set_transmitting(false);
	}
}

void EncodersView::start_tx(const bool scan) {
	size_t bitstream_length = 0;

	repeat_min = view_config.repeat_min();

	if (scan) {
		tx_mode = SCAN;
		int n = view_scan.field_length.value();
		uint8_t *bitstream = shared_memory.bb_data.data;
		const size_t size = (1 << (n - 1)) / 4;
		memset(bitstream, 0, size);
		bitstream_length = view_scan.scan_sequence(n, bitstream);
	} else {
		tx_mode = SINGLE;
		view_config.generate_frame();
		bitstream_length = make_bitstream(view_config.frame_fragments);
	}

	repeat_index = 1;
	progressbar.set_max(repeat_min);
	update_progress();

	transmitter_model.set_sampling_rate(OOK_SAMPLERATE);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();

	baseband::set_ook_data(
		bitstream_length,
		view_config.samples_per_bit(),
		repeat_min,
		view_config.pause_symbols()
	);
}

EncodersView::EncodersView(
	NavigationView& nav
) : nav_ { nav }
{
	baseband::run_image(portapack::spi_flash::image_tag_ook);

	add_children({
		&tab_view,
		&view_config,
		&view_scan,
		&text_status,
		&progressbar,
		&tx_view
	});

	// load app settings
	auto rc = settings.load("tx_ook", &app_settings);
	if (rc == SETTINGS_OK) {
		transmitter_model.set_rf_amp(app_settings.tx_amp);
		transmitter_model.set_channel_bandwidth(app_settings.channel_bandwidth);
		transmitter_model.set_tuning_frequency(app_settings.tx_frequency);
		transmitter_model.set_tx_gain(app_settings.tx_gain);
	}

	tx_view.on_edit_frequency = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			transmitter_model.set_tuning_frequency(f);
		};
	};

	tx_view.on_start = [this]() {
		tx_view.set_transmitting(true);
		start_tx(tab_view.selected());
	};

	tx_view.on_stop = [this]() {
		tx_view.set_transmitting(false);
	};
}

} /* namespace ui */
