/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2022 José Moreira
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

using namespace portapack;

namespace ui
{
	EncodersView::EncodersView(
		NavigationView &nav) : nav_{nav}
	{
		baseband::run_image(portapack::spi_flash::image_tag_ook);

		transmitter_model.set_sampling_rate(OOK_SAMPLERATE);
		transmitter_model.set_baseband_bandwidth(1750000);

		// Default encoder def
		encoder_def = &encoder_defs[0];

		using option_t = std::pair<std::string, int32_t>;
		std::vector<option_t> enc_options;
		size_t i;

		add_children({&labels,
					  &options_encoder,
					  &options_tx_method,
					  &field_clk,
					  &field_frameduration,
					  &field_repeat_min,
					  &checkbox_reversed,
					  &symfield_word,
					  &text_format,
					  &waveform,
					  &text_progress,
					  &progress_bar,
					  &tx_view});

		options_encoder.on_change = [this](size_t index, int32_t)
		{
			encoder_def = &encoder_defs[index];

			field_clk.set_value(encoder_def->default_clk_speed / 1000);
			field_repeat_min.set_value(encoder_def->repeat_min);

			// reset reversed checkbox
			checkbox_reversed.set_value(false);

			// reset the debruijn sequencer in case the encoder is vulnerable
			if (encoder_def->is_vuln_to_debruijn)
			{
				debruijn_seq.init(encoder_def->word_length);
			}

			reset_symfield();
			generate_frame();
			draw_waveform();
			check_if_encoder_is_vuln_to_debruijn();
			check_if_encoder_can_be_reversed();
		};

		options_tx_method.on_change = [this](size_t, int32_t)
		{
			reset_symfield();
			generate_frame();
			draw_waveform();
			check_if_encoder_is_vuln_to_debruijn();
		};

		symfield_word.on_change = [this]()
		{
			generate_frame();
			draw_waveform();
		};

		// whenever the checkbox changes, rerender the waveform
		checkbox_reversed.on_select = [this](Checkbox &, bool)
		{
			generate_frame();
			draw_waveform();
		};

		// Selecting input clock changes symbol and word duration
		field_clk.on_change = [this](int32_t value)
		{
			// value is in kHz, new_value is in us
			int32_t new_value = (encoder_def->clk_per_symbol * 1000000) / (value * 1000);
			if (new_value != field_frameduration.value())
				field_frameduration.set_value(new_value * encoder_def->word_length, false);
		};

		// Selecting word duration changes input clock and symbol duration
		field_frameduration.on_change = [this](int32_t value)
		{
			// value is in us, new_value is in kHz
			int32_t new_value = (value * 1000) / (encoder_def->word_length * encoder_def->clk_per_symbol);
			if (new_value != field_clk.value())
				field_clk.set_value(1000000 / new_value, false);
		};

		tx_view.on_edit_frequency = [this, &nav]()
		{
			auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.tuning_frequency());
			new_view->on_changed = [this](rf::Frequency f)
			{
				transmitter_model.set_tuning_frequency(f);
			};
		};

		tx_view.on_start = [this]()
		{
			tx_view.set_transmitting(true);

			// Start transmitting
			switch (options_tx_method.selected_index_value())
			{
			case TX_MODE_MANUAL:
				start_single_tx();
				break;
			case TX_MODE_DEBRUIJN:
				start_debruijn_tx();
				break;
			case TX_MODE_BRUTEFORCE:
				start_bruteforce_tx();
				break;
			}
		};

		tx_view.on_stop = [this]()
		{
			stop_tx();
		};

		// Load encoder types in option field
		for (i = 0; i < ENC_TYPES_COUNT; i++)
			enc_options.emplace_back(std::make_pair(encoder_defs[i].name, i));
		options_encoder.set_options(enc_options);
		options_encoder.set_selected_index(0);
	}

	void
	EncodersView::focus()
	{
		options_encoder.focus();
	}

	void EncodersView::check_if_encoder_is_vuln_to_debruijn()
	{
		// if the selected tx method is DEBRUIJN, check if the encoder is vulnerable to DEBRUIJN
		if (options_tx_method.selected_index_value() == TX_MODE_DEBRUIJN && !encoder_def->is_vuln_to_debruijn)
		{
			text_progress.set_style(&style_err);
			text_progress.set("Not vuln to DeBruijn");
			tx_view.set_focusable(false);
			return;
		}

		tx_view.set_focusable(true);
		update_progress();
	}

	void EncodersView::check_if_encoder_can_be_reversed()
	{
		// if the selected tx method is DEBRUIJN, check if the encoder is vulnerable to DEBRUIJN
		if (sizeof(encoder_def->symbols_bit_fragments) == 2)
		{
			checkbox_reversed.set_focusable(true);
		}
		else
		{
			checkbox_reversed.set_focusable(false);
			checkbox_reversed.set_value(false);
		}
	}

	void EncodersView::reset_symfield()
	{
		char symbol_type;
		std::string format_string = "";
		size_t word_length = encoder_def->word_length;

		symfield_word.set_length(word_length);

		size_t n = 0, i = 0;
		while (n < word_length)
		{
			symbol_type = encoder_def->word_format[i++];
			if (symbol_type == 'A')
			{
				symfield_word.set_symbol_list(n++, encoder_def->symfield_address_symbols);
				format_string += 'A';
			}
			else if (symbol_type == 'D')
			{
				symfield_word.set_symbol_list(n++, encoder_def->symfield_data_symbols);
				format_string += 'D';
			}
		}

		// Ugly :( Pad to erase
		format_string.append(24 - format_string.size(), ' ');

		text_format.set(format_string);
	}

	const char *EncodersView::get_symbols_bit_fragments(const uint32_t index)
	{
		if (checkbox_reversed.value())
		{
			return encoder_def->symbols_bit_fragments[(index == 0) ? 1 : 0];
		}

		return encoder_def->symbols_bit_fragments[index];
	}

	void EncodersView::generate_frame()
	{
		uint8_t i = 0;

		int32_t mode = (tx_mode != TX_MODE_IDLE) ? static_cast<int32_t>(tx_mode) : options_tx_method.selected_index_value();
		frame_fragments.clear();

		if (mode == TX_MODE_MANUAL || mode == TX_MODE_BRUTEFORCE)
		{
			for (i = 0; i < encoder_def->word_length; i++)
			{
				if (encoder_def->word_format[i] == 'S')
					frame_fragments += encoder_def->sync_bit_fragment;
				else
					frame_fragments += get_symbols_bit_fragments(symfield_word.get_sym(i));
			}
		}

		if (mode == TX_MODE_DEBRUIJN)
		{ // De Bruijn!
			if (!encoder_def->is_vuln_to_debruijn)
			{
				frame_fragments = "0";
				return;
			}

			for (i = 0; i < 32; i++)
			{
				if (debruijn_bits & (1 << (31 - i)))
					frame_fragments += get_symbols_bit_fragments(1);
				else
					frame_fragments += get_symbols_bit_fragments(0);
			}
		}
	}

	void EncodersView::draw_waveform()
	{
		size_t length = frame_fragments.length();

		for (size_t n = 0; n < length; n++)
			waveform_buffer[n] = (frame_fragments[n] == '0') ? 0 : 1;

		waveform.set_length(length);
		waveform.set_dirty();
	}

	uint32_t EncodersView::samples_per_bit()
	{
		return OOK_SAMPLERATE / ((field_clk.value() * 1000) / (encoder_def->clk_per_symbol / encoder_def->bit_fragments_length_per_symbol));
	}

	EncodersView::~EncodersView()
	{
		transmitter_model.disable();
		baseband::shutdown();
	}

	// NOTE: should be called after changing the tx_mode
	void EncodersView::init_progress()
	{
		progress_bar.set_max(tx_repeat_min);
		update_progress();
	}

	void EncodersView::update_progress()
	{
		std::string str_buffer;

		if (tx_mode == TX_MODE_MANUAL)
		{
			text_progress.set_style(&style_info);
			str_buffer = "Manual: " + to_string_dec_uint(tx_repeat_index) + "/" + to_string_dec_uint(tx_repeat_min);
			text_progress.set(str_buffer);
			progress_bar.set_value(tx_repeat_index);
		}

		if (tx_mode == TX_MODE_DEBRUIJN)
		{
			text_progress.set_style(&style_info);
			str_buffer = "De Bruijn: " + to_string_dec_uint(debruijn_index) + "/" + to_string_dec_uint(debruijn_max);
			text_progress.set(str_buffer);
			progress_bar.set_value(debruijn_index);
		}

		if (tx_mode == TX_MODE_BRUTEFORCE)
		{
			text_progress.set_style(&style_info);
			str_buffer = to_string_dec_uint(bruteforce_index) + "/" + to_string_dec_uint(bruteforce_max) + " (" + to_string_dec_uint(tx_repeat_index) + "/" + to_string_dec_uint(tx_repeat_min) + ")";
			text_progress.set(str_buffer);
			progress_bar.set_value(debruijn_index);
		}

		if (tx_mode == TX_MODE_IDLE)
		{
			text_progress.set_style(&style_success);
			text_progress.set("Ready");
			progress_bar.set_value(0);
		}
	}

	void EncodersView::on_tx_progress(const uint32_t progress, const bool done)
	{
		if (tx_mode == TX_MODE_MANUAL)
		{
			if (done)
			{
				stop_tx();
				return;
			}

			// Repeating...
			tx_repeat_index = progress + 1;
			update_progress();
		}

		if (tx_mode == TX_MODE_DEBRUIJN)
		{
			// NOTE: done shoul always be true, so to assess if this is done, we need to check the
			// index instead and compare it with the count

			debruijn_index++;

			// emit the next packet
			tick_debruijn_tx();
		}

		if (tx_mode == TX_MODE_BRUTEFORCE)
		{
			if (done)
			{
				tick_bruteforce_tx();
				return;
			}

			// Repeating...
			tx_repeat_index = progress + 1;
			update_progress();
		}
	}

	void EncodersView::start_single_tx()
	{
		tx_mode = TX_MODE_MANUAL;
		tx_repeat_index = 1;
		tx_repeat_min = field_repeat_min.value();
		init_progress();

		afsk_repeats = tx_repeat_min;
		tx();
	}

	void EncodersView::start_debruijn_tx()
	{
		// before procesing, check if the encoder is vulneravle to Debruijn
		if (!encoder_def->is_vuln_to_debruijn)
		{
			stop_tx();
			check_if_encoder_is_vuln_to_debruijn();
			return;
		}

		symfield_word.set_focusable(false);

		debruijn_index = 0; // Scanning, and this is first time
		tx_mode = TX_MODE_DEBRUIJN;
		init_progress();
		tick_debruijn_tx();
	}

	void EncodersView::tick_debruijn_tx()
	{

		if (debruijn_index == 0)
		{
			uint32_t debruijn_total = debruijn_seq.init(encoder_def->word_length);
			debruijn_max = (debruijn_total / encoder_def->word_length) + 1; // get total number of packets to tx, plus one, be sure of sending whatever is left from division
		}

		if (debruijn_index == debruijn_max)
		{
			stop_tx();
			return;
		}

		debruijn_bits = debruijn_seq.compute(encoder_def->word_length); // bits sequence for this step

		afsk_repeats = 1;
		tx();
	}

	void EncodersView::start_bruteforce_tx()
	{
		symfield_word.set_focusable(false);

		tx_mode = TX_MODE_BRUTEFORCE;
		tx_repeat_index = 1;
		tx_repeat_min = field_repeat_min.value();
		bruteforce_index = symfield_word.get_possibility();
		bruteforce_max = symfield_word.get_possibilities_count();
		init_progress();
		afsk_repeats = tx_repeat_min;

		tx();
	}

	void EncodersView::tick_bruteforce_tx()
	{
		if (bruteforce_index == bruteforce_max)
		{
			stop_tx();
			return;
		}

		symfield_word.set_next_possibility();
		bruteforce_index++;
		tx();
	}

	void EncodersView::tx()
	{
		generate_frame();
		draw_waveform();
		update_progress();

		size_t bitstream_length = make_bitstream(frame_fragments);

		transmitter_model.enable();
		baseband::set_ook_data(
			bitstream_length,
			samples_per_bit(),
			afsk_repeats,
			encoder_def->pause_symbols);
	}

	void EncodersView::stop_tx()
	{
		transmitter_model.disable();
		tx_mode = TX_MODE_IDLE;
		text_progress.set("Done");
		tx_view.set_transmitting(false);
		symfield_word.set_focusable(true);

		if (encoder_def->is_vuln_to_debruijn)
		{
			debruijn_seq.init(encoder_def->word_length);
		}

		tx_repeat_index = 0;
		tx_repeat_min = 0;
		init_progress();
		generate_frame();
		draw_waveform();
	}
} /* namespace ui */
