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
					  &symfield_word,
					  &text_format,
					  &waveform,
					  &text_progress,
					  &progress_bar,
					  &tx_view});

		options_encoder.on_change = [this](size_t index, int32_t)
		{
			on_encoder_change(index);
		};

		options_tx_method.on_change = [this](size_t, int32_t selected)
		{
			on_tx_method_change(selected);
		};

		symfield_word.on_change = [this]()
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

	void EncodersView::focus()
	{
		options_encoder.focus();
	}

	void EncodersView::on_tx_method_change(int32_t selected_tx_mode)
	{
		switch (selected_tx_mode)
		{
		case TX_MODE_MANUAL:
			// show the symbols field
			symfield_word.set_focusable(true);
			break;
		case TX_MODE_DEBRUIJN:
		case TX_MODE_BRUTEFORCE:
			// hide the symbols field
			symfield_word.set_focusable(false);
			break;
		}

		reset_symfield();
		generate_frame();
		draw_waveform();
	}

	void EncodersView::on_encoder_change(size_t index)
	{
		encoder_def = &encoder_defs[index];

		field_clk.set_value(encoder_def->default_speed / 1000);
		field_repeat_min.set_value(encoder_def->repeat_min);

		reset_symfield();
		generate_frame();
		draw_waveform();
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
				symfield_word.set_symbol_list(n++, encoder_def->address_symbols);
				format_string += 'A';
			}
			else if (symbol_type == 'D')
			{
				symfield_word.set_symbol_list(n++, encoder_def->data_symbols);
				format_string += 'D';
			}
		}

		// Ugly :( Pad to erase
		format_string.append(24 - format_string.size(), ' ');

		text_format.set(format_string);
	}

	void EncodersView::generate_frame()
	{
		uint8_t i = 0;

		int32_t mode = (tx_mode != TX_MODE_IDLE) ? tx_mode : options_tx_method.selected_index_value();
		frame_fragments.clear();

		if (mode == TX_MODE_MANUAL || mode == TX_MODE_BRUTEFORCE)
		{
			for (auto c : encoder_def->word_format)
			{
				if (c == 'S')
					frame_fragments += encoder_def->sync;
				else
					frame_fragments += encoder_def->bit_format[symfield_word.get_sym(i++)];
			}
		}

		if (mode == TX_MODE_DEBRUIJN)
		{ // De Bruijn!
			char *word_ptr = (char *)encoder_def->word_format;
			uint8_t pos = 0;

			while (*word_ptr)
			{

				if (*word_ptr == 'S')
					frame_fragments += encoder_def->sync;
				else if (*word_ptr == 'D')
					frame_fragments += encoder_def->bit_format[symfield_word.get_sym(i++)]; // Get_sym brings the index of the char chosen in the symfield, so 0, 1 or eventually 2
				else
				{

					if (debruijn_bits & (1 << (31 - pos)))
						frame_fragments += encoder_def->bit_format[1];
					else
						frame_fragments += encoder_def->bit_format[0];

					pos--;
					i++; // Even while grabbing this address bit from debruijn, must move forward on the symfield, in case there is a 'D' further ahead
				}

				word_ptr++;
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
		return OOK_SAMPLERATE / ((field_clk.value() * 1000) / encoder_def->clk_per_fragment);
	}

	// uint16_t EncodersView::repeat_skip_bits_count()
	// {
	// 	return encoder_def->skip_repeat_bits ? strlen(encoder_def->sync) : 0;
	// }

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
			str_buffer = "Manual: " + to_string_dec_uint(tx_repeat_index) + "/" + to_string_dec_uint(tx_repeat_min);
			text_progress.set(str_buffer);
			progress_bar.set_value(tx_repeat_index);
		}

		if (tx_mode == TX_MODE_DEBRUIJN)
		{
			str_buffer = "De Bruijn: " + to_string_dec_uint(debruijn_index) + "/" + to_string_dec_uint(debruijn_max);
			text_progress.set(str_buffer);
			progress_bar.set_value(debruijn_index);
		}

		if (tx_mode == TX_MODE_BRUTEFORCE)
		{
			str_buffer = to_string_dec_uint(bruteforce_index) + "/" + to_string_dec_uint(bruteforce_max) + " (" + to_string_dec_uint(tx_repeat_index) + "/" + to_string_dec_uint(tx_repeat_min) + ")";
			text_progress.set(str_buffer);
			progress_bar.set_value(debruijn_index);
		}

		if (tx_mode == TX_MODE_IDLE)
		{
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

			debruijn_index = debruijn_index + 1;

			if (debruijn_index == debruijn_max)
			{
				stop_tx();
				return;
			}

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
		debruijn_index = 0; // Scanning, and this is first time
		tx_mode = TX_MODE_DEBRUIJN;
		init_progress();
		tick_debruijn_tx();
	}

	void EncodersView::tick_debruijn_tx()
	{
		if (debruijn_index == 0)
		{
			debruijn_bits_per_packet = 0; // Determine the A (Addresses) bit quantity
			for (uint8_t c = 0; c < encoder_def->word_length; c++)
				if (encoder_def->word_format[c] == 'A') // Address bit found
					debruijn_bits_per_packet++;

			uint32_t debruijn_total = debruijn_seq.init(debruijn_bits_per_packet);
			debruijn_max = (debruijn_total / debruijn_bits_per_packet) + 1; // get total number of packets to tx, plus one, be sure of sending whatever is left from division
		}

		debruijn_bits = debruijn_seq.compute(debruijn_bits_per_packet); // bits sequence for this step

		afsk_repeats = 1;
		tx();
	}

	void EncodersView::start_bruteforce_tx()
	{
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

		transmitter_model.set_sampling_rate(OOK_SAMPLERATE);
		transmitter_model.set_rf_amp(true);
		transmitter_model.set_baseband_bandwidth(1750000);
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

		tx_repeat_index = 0;
		tx_repeat_min = 0;
		init_progress();
	}
} /* namespace ui */
