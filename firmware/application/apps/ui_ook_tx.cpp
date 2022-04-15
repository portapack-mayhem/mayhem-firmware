/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2022 José Moreira @cusspvz
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

#include "ui_ook_tx.hpp"

#include "baseband_api.hpp"
#include "string_format.hpp"

using namespace portapack;

namespace ui
{

	// 	///////////////////////////////////////////////////////////////////////////////
	// 	// OOKTxGeneratorView

	OOKTxGeneratorView::OOKTxGeneratorView(NavigationView &nav, Rect parent_rect)
	{
		set_parent_rect(parent_rect);
		hidden(true);

		using option_t = std::pair<std::string, int32_t>;
		std::vector<option_t> enc_options;
		size_t i;

		// Default encoder def
		encoder_def = &encoder_defs[0];

		// Load encoder types in option field
		for (i = 0; i < ENC_TYPES_COUNT; i++)
			enc_options.emplace_back(std::make_pair(encoder_defs[i].name, i));

		add_children({
			&labels,
			&options_encoder,
			&options_tx_method,
			&field_clk,
			&field_frameduration,
			&options_period_per_symbol,
			&field_repeat_min,
			&field_pause_symbols,
			&symfield_word,
			&text_format,
		});

		// options_encoder.on_change = [this](size_t index, int32_t)
		// {
		// 	encoder_def = &encoder_defs[index];

		// 	field_clk.set_value(encoder_def->default_clk_speed / 1000);
		// 	field_repeat_min.set_value(encoder_def->repeat_min);
		// 	options_period_per_symbol.set_by_value(encoder_def->period_per_symbol);
		// 	field_pause_symbols.set_value(encoder_def->pause_symbols);

		// 	reset_symfield();
		// 	check_if_encoder_is_vuln_to_debruijn();
		// 	check_if_encoder_can_be_reversed();

		// 	// reset the debruijn sequencer in case the encoder is vulnerable
		// 	if (encoder_def->is_vuln_to_debruijn)
		// 	{
		// 		reset_debruijn();
		// 	}

		// 	if (on_encoder_change)
		// 		on_encoder_change();

		// 	if (on_waveform_change_request)
		// 		on_waveform_change_request();
		// };

		// options_tx_method.on_change = [this](size_t, int32_t value)
		// {
		// 	if (value == TX_MODE_DEBRUIJN)
		// 	{
		// 		// Set the repeat to 0
		// 		field_repeat_min.set_value(0);
		// 	}
		// 	else
		// 	{
		// 		// set to the default repeat min at the selected encoder
		// 		field_repeat_min.set_value(encoder_def->repeat_min);
		// 	}

		// 	reset_symfield();
		// 	check_if_encoder_is_vuln_to_debruijn();

		// 	if (on_waveform_change_request)
		// 		on_waveform_change_request();
		// };

		// symfield_word.on_change = [this]()
		// {
		// 	if (on_waveform_change_request)
		// 		on_waveform_change_request();
		// };

		// // Selecting input clock changes symbol and word duration
		// field_clk.on_change = [this](int32_t value)
		// {
		// 	// value is in kHz, new_value is in us
		// 	int32_t new_value = (options_period_per_symbol.selected_index_value() * 1000000) / (value * 1000);
		// 	if (new_value != field_frameduration.value())
		// 		field_frameduration.set_value(new_value * encoder_def->word_length, false);
		// };

		// // Selecting word duration changes input clock and symbol duration
		// field_frameduration.on_change = [this](int32_t value)
		// {
		// 	// value is in us, new_value is in kHz
		// 	int32_t new_value = (value * 1000) / (encoder_def->word_length * options_period_per_symbol.selected_index_value());
		// 	if (new_value != field_clk.value())
		// 		field_clk.set_value(1000000 / new_value, false);
		// };

		// options_period_per_symbol.on_change = [this](size_t, int32_t)
		// {
		// 	// trigger the change on both fields
		// 	field_clk.on_change(field_clk.value());
		// 	field_frameduration.on_change(field_frameduration.value());
		// };

		options_encoder.set_options(enc_options);
	}

	void OOKTxGeneratorView::focus()
	{
		options_encoder.focus();
	}

	void OOKTxGeneratorView::on_show()
	{
		options_encoder.set_selected_index(0);
	}

	// 	void OOKTxGeneratorView::check_if_encoder_is_vuln_to_debruijn()
	// 	{
	// 		// if the selected tx method is DEBRUIJN, check if the encoder is vulnerable to DEBRUIJN
	// 		if (options_tx_method.selected_index_value() == TX_MODE_DEBRUIJN && !encoder_def->is_vuln_to_debruijn)
	// 		{
	// 			on_status_change("Not vuln to DeBruijn");
	// 			return;
	// 		}

	// 		on_status_change("");
	// 	}

	// 	void OOKTxGeneratorView::check_if_encoder_can_be_reversed()
	// 	{
	// 		// if the selected tx method is DEBRUIJN, check if the encoder is vulnerable to DEBRUIJN
	// 		if (sizeof(encoder_def->symbols_bit_fragments) == 2)
	// 		{
	// 			// checkbox_reversed.set_focusable(true);
	// 		}
	// 		else
	// 		{
	// 			// checkbox_reversed.set_focusable(false);
	// 			// checkbox_reversed.set_value(false);
	// 		}
	// 	}

	// 	void OOKTxGeneratorView::reset_symfield()
	// 	{
	// 		char symbol_type;
	// 		std::string format_string = "";
	// 		size_t word_length = encoder_def->word_length;

	// 		symfield_word.set_length(word_length);

	// 		size_t n = 0, i = 0;
	// 		while (n < word_length)
	// 		{
	// 			symbol_type = encoder_def->word_format[i++];
	// 			if (symbol_type == 'A')
	// 			{
	// 				symfield_word.set_symbol_list(n++, encoder_def->symfield_address_symbols);
	// 				format_string += 'A';
	// 			}
	// 			else if (symbol_type == 'D')
	// 			{
	// 				symfield_word.set_symbol_list(n++, encoder_def->symfield_data_symbols);
	// 				format_string += 'D';
	// 			}
	// 		}

	// 		// Ugly :( Pad to erase
	// 		format_string.append(24 - format_string.size(), ' ');

	// 		text_format.set(format_string);
	// 	}

	// 	void OOKTxGeneratorView::reset_debruijn()
	// 	{
	// 		if (
	// 			debruijn_sequencer.k == 2 &&
	// 			debruijn_sequencer.n == encoder_def->word_length &&
	// 			debruijn_sequencer.sequence.length() > 0)
	// 			return;

	// 		debruijn_sequencer.init("01", encoder_def->word_length, 32);
	// 		debruijn_sequencer.generate();
	// 	}

	// 	const char *OOKTxGeneratorView::get_symbols_bit_fragments(const uint8_t index, const bool reversed)
	// 	{
	// 		if (reversed)
	// 		{
	// 			return encoder_def->symbols_bit_fragments[(index == 0) ? 1 : 0];
	// 		}

	// 		return encoder_def->symbols_bit_fragments[index];
	// 	}

	// 	uint16_t OOKTxGeneratorView::get_repeat_total()
	// 	{
	// 		switch (options_tx_method.selected_index_value())
	// 		{
	// 		case TX_MODE_MANUAL:
	// 		case TX_MODE_BRUTEFORCE:
	// 			return field_repeat_min.value();

	// 		case TX_MODE_DEBRUIJN:
	// 			return 1;
	// 		}
	// 	}

	// 	uint16_t OOKTxGeneratorView::get_frame_part_total()
	// 	{
	// 		switch (options_tx_method.selected_index_value())
	// 		{
	// 		case TX_MODE_BRUTEFORCE:
	// 			return symfield_word.get_possibilities_count();

	// 		case TX_MODE_DEBRUIJN: // check the size of the whole debruijn sequence
	// 		case TX_MODE_MANUAL:
	// 			return 1;
	// 		}
	// 	}

	std::string OOKTxGeneratorView::generate_frame_part(const uint16_t frame_part_index, const bool reversed)
	{
		int32_t mode = options_tx_method.selected_index_value();

		// 		// NOTE: bruteforce might need to be check to enforce the frame_part_index
		// 		// TODO: :this:

		// 		if (mode == TX_MODE_MANUAL || mode == TX_MODE_BRUTEFORCE)
		// 		{
		// 			std::string frame_fragments = "";

		// 			for (uint8_t i = 0; i < encoder_def->word_length; i++)
		// 				if (encoder_def->word_format[i] == 'S')
		// 					frame_fragments += encoder_def->sync_bit_fragment;
		// 				else
		// 					frame_fragments += get_symbols_bit_fragments(symfield_word.get_sym(i), reversed);

		// 			return frame_fragments;
		// 		}

		// 		if (mode == TX_MODE_DEBRUIJN)
		// 		{
		// 			if (!encoder_def->is_vuln_to_debruijn)
		// 				return "0";

		// 			std::string frame_fragments = "";
		// 			std::string frame_part = debruijn_sequencer.get_part(frame_part_index);

		// 			for (uint8_t i = 0; i < frame_part.length(); i++)
		// 			{
		// 				frame_fragments += get_symbols_bit_fragments(("1" == &frame_part.at(i)) ? 1 : 0, reversed);
		// 			}

		// 			return frame_fragments;
		// 		}

		return "0";
	}

	// 	uint32_t OOKTxGeneratorView::samples_per_bit()
	// 	{
	// 		return OOK_SAMPLERATE / ((field_clk.value() * 1000) / (options_period_per_symbol.selected_index_value() / encoder_def->bit_fragments_length_per_symbol));
	// 	}

	// 	///////////////////////////////////////////////////////////////////////////////
	// 	// OOKTxLoaderView

	OOKTxLoaderView::OOKTxLoaderView(
		NavigationView &, Rect parent_rect)
	{
		set_parent_rect(parent_rect);
		hidden(true);

		add_children({
			&labels,
		});
	}

	void OOKTxLoaderView::focus()
	{
		// field_debug.focus();
	}

	// 	uint16_t OOKTxLoaderView::get_repeat_total() { return 1; }
	// 	uint16_t OOKTxLoaderView::get_frame_part_total() { return 1; }

	std::string OOKTxLoaderView::generate_frame_part(const uint16_t frame_part_index, const bool reversed)
	{
		return "0";
	}

	///////////////////////////////////////////////////////////////////////////////
	// OOKTxDeBruijnView

	OOKTxDeBruijnView::OOKTxDeBruijnView(
		NavigationView &, Rect parent_rect)
	{
		set_parent_rect(parent_rect);
		hidden(true);

		add_children({
			&labels,
			&field_init,
			&field_compute,
			&text_debug,
			&text_length,
		});

		// 		field_init.on_change = [this](int32_t value)
		// 		{
		// 			debruijn_sequencer.init("01", value, 32);
		// 			debruijn_sequencer.generate();
		// 			text_length.set(to_string_dec_uint(debruijn_sequencer.length));
		// 			field_compute.set_value(0);

		// 			if (on_waveform_change_request)
		// 				on_waveform_change_request();
		// 		};

		// 		field_compute.on_change = [this](int32_t offset)
		// 		{
		// 			text_debug.set((debruijn_sequencer.sequence.length() > offset + 25) ? debruijn_sequencer.sequence.substr(offset, 25) : debruijn_sequencer.sequence);
		// 		};
	}

	void OOKTxDeBruijnView::focus()
	{
		field_init.focus();
	}

	// 	uint16_t OOKTxDeBruijnView::get_repeat_total() { return 1; }
	// 	uint16_t OOKTxDeBruijnView::get_frame_part_total() { return 1; }

	std::string OOKTxDeBruijnView::generate_frame_part(const uint16_t frame_part_index, const bool reverse)
	// TODO: we still need to implement the reverse flag
	{
		return debruijn_sequencer.get_part(frame_part_index);
	}

	// 	///////////////////////////////////////////////////////////////////////////////
	// 	// OOKTxView

	OOKTxView::OOKTxView(NavigationView &nav) : nav_{nav}
	{
		// baseband::run_image(portapack::spi_flash::image_tag_ook);

		// transmitter_model.set_sampling_rate(OOK_SAMPLERATE);
		// transmitter_model.set_baseband_bandwidth(1750000);

		add_children({
			&tab_view,

			// tab views
			&view_debruijn,
			&view_loader,
			&view_generator,

			&checkbox_reversed,
			&waveform,
			&text_progress,
			&progress_bar,
			&tx_view,
		});

		// Event listeners
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
			start_tx();
		};

		tx_view.on_stop = [this]()
		{
			stop_tx();
		};

		// whenever the checkbox changes, rerender the waveform
		checkbox_reversed.on_select = [this](Checkbox &, bool)
		{
			generate_frame_part();
			draw_waveform();
		};

		// View hooks

		view_debruijn.on_waveform_change_request =
			view_loader.on_waveform_change_request =
				view_generator.on_waveform_change_request = [this]()
		{
			generate_frame_part();
			draw_waveform();
		};

		// view_debruijn.on_status_change =
		// 	view_loader.on_status_change =
		// 		view_generator.on_status_change = [this](const std::string e)
		// {
		// 	if (err != e)
		// 	{
		// 		err = e;
		// 		progress_update();
		// 	}
		// };

		// view_generator.on_encoder_change = [this]()
		// {
		// 	// reset reversed checkbox
		// 	checkbox_reversed.set_value(false);
		// };
	}

	void OOKTxView::focus()
	{
		// tab_view.focus();
	}

	OOKTxView::~OOKTxView()
	{
		transmitter_model.disable();
		baseband::shutdown();
	}

	// TX methods
	void OOKTxView::generate_frame_part()
	{
		frame_fragments.clear();

		if (tab_view.selected() == 0)
			frame_fragments = view_loader.generate_frame_part(
				tx_mode,
				checkbox_reversed.value());

		if (tab_view.selected() == 1)
			frame_fragments = view_generator.generate_frame_part(
				tx_mode,
				checkbox_reversed.value());

		if (tab_view.selected() == 2)
			frame_fragments = view_debruijn.generate_frame_part(
				tx_mode,
				checkbox_reversed.value());
	}

	// NOTE: should be called after changing the tx_mode
	void OOKTxView::progress_reset()
	{
		// progress_bar.set_max(frame_parts_cursor.total * repeat_cursor.total);
		progress_update();
	}

	void OOKTxView::progress_update()
	{
		if (err != "")
		{
			text_progress.set_style(&style_err);
			text_progress.set(err);
		}
		else if (tx_mode == TX_MODE_IDLE)
		{
			text_progress.set_style(&style_success);
			text_progress.set("Ready");
			progress_bar.set_value(0);
		}
		else
		{
			text_progress.set_style(&style_info);
			// text_progress.set(
			// 	to_string_dec_uint(frame_parts_cursor.index) + "/" + to_string_dec_uint(frame_parts_cursor.total) +
			// 	" (" + to_string_dec_uint(repeat_cursor.index) + "/" + to_string_dec_uint(repeat_cursor.total) + ")");
		}

		// progress_bar.set_value(frame_parts_cursor.index * repeat_cursor.total + repeat_cursor.index);
	}

	void OOKTxView::start_tx()
	{
		// 		reset_cursors();

		// 		switch (tab_view.selected())
		// 		{
		// 		// Loader View TX
		// 		case 0:
		// 			break;

		// 		// Generator View TX
		// 		case 1:
		// 			switch (view_generator.options_tx_method.selected_index_value())
		// 			{
		// 			case TX_MODE_MANUAL:
		// 				tx_mode = TX_MODE_MANUAL;
		// 				break;

		// 			case TX_MODE_DEBRUIJN:
		// 				if (!view_generator.encoder_def->is_vuln_to_debruijn)
		// 				{
		// 					stop_tx();
		// 					view_generator.check_if_encoder_is_vuln_to_debruijn();
		// 					return;
		// 				}

		// 				tx_mode = TX_MODE_DEBRUIJN;
		// 				view_generator.symfield_word.set_focusable(false);
		// 				view_generator.reset_debruijn();
		// 				break;

		// 			case TX_MODE_BRUTEFORCE:
		// 				tx_mode = TX_MODE_BRUTEFORCE;
		// 				// frame_parts_cursor.index = view_generator.symfield_word.get_possibility();
		// 				view_generator.symfield_word.set_focusable(false);
		// 				break;
		// 			}
		// 			break;

		// 		// DeBruijn View TX
		// 		case 2:
		// 			break;
		// 		}

		// 		tx_view.set_transmitting(true);
		// 		progress_reset();
		// 		tx();
	}

	// 	// this method captures a tx message and decides what to do next.
	// 	// ideally, it should attempt first to repeat the frame as repetitions are
	// 	// handled by the baseband logic and then it should attempt to send the other
	// 	// frame parts.

	void OOKTxView::on_tx_progress(const uint32_t, const bool done)
	{
		// repeat_cursor.index++;

		// // check if we have other frames to transmit or if we want to stop transmission
		// if (done)
		// {
		// 	if (frame_parts_cursor.is_done())
		// 	{
		// 		stop_tx();
		// 		return;
		// 	}

		// 	if (tab_view.selected() == TX_MODE_BRUTEFORCE)
		// 	{
		// 		view_generator.symfield_word.set_next_possibility();
		// 	}

		// 	frame_parts_cursor.index++;
		// }

		// // call tx
		// tx();
	}

	void OOKTxView::tx()
	{
		// 		generate_frame_part();
		// 		draw_waveform();
		// 		progress_update();

		// 		size_t bitstream_length = make_bitstream(frame_fragments);

		// 		transmitter_model.enable();
		// 		baseband::set_ook_data(
		// 			bitstream_length,
		// 			view_generator.samples_per_bit(),
		// 			1,
		// 			// repeat_cursor.total,
		// 			view_generator.field_pause_symbols.value());
	}

	void OOKTxView::stop_tx()
	{
		// 		transmitter_model.disable();
		// 		tx_mode = TX_MODE_IDLE;
		// 		text_progress.set("Done");
		// 		tx_view.set_transmitting(false);
		// 		view_generator.symfield_word.set_focusable(true);

		// 		reset_cursors();
		// 		progress_reset();
		// 		generate_frame_part();
		// 		draw_waveform();
	}

	void OOKTxView::reset_cursors()
	{
		// 		// repeat_cursor.reset();
		// 		// frame_parts_cursor.reset();

		// 		// // get the correct repeat min
		// 		// repeat_cursor.total = view_generator.field_repeat_min.value();

		// 		// // Generator
		// 		// switch (tab_view.selected())
		// 		// {
		// 		// // Loader View TX
		// 		// case 0:
		// 		// 	repeat_cursor.total = view_loader.get_repeat_total();
		// 		// 	frame_parts_cursor.total = view_loader.get_frame_part_total();
		// 		// 	break;

		// 		// // Generator View TX
		// 		// case 1:
		// 		// 	repeat_cursor.total = view_generator.get_repeat_total();
		// 		// 	frame_parts_cursor.total = view_generator.get_frame_part_total();
		// 		// 	break;

		// 		// // DeBruijn TX
		// 		// case 2:
		// 		// 	repeat_cursor.total = view_generator.get_repeat_total();
		// 		// 	frame_parts_cursor.total = view_generator.get_frame_part_total();
		// 		// 	break;
		// 		// }
	}

	void OOKTxView::draw_waveform()
	{
		size_t length = frame_fragments.length();
		bool is_dirty = false;
		int16_t tmp_bit;

		for (int32_t n = 0; n < length; n++)
		{
			tmp_bit = frame_fragments[n];

			if (tmp_bit != waveform_buffer[n])
			{
				waveform_buffer[n] = tmp_bit;
				is_dirty = true;
			}
		}

		if (waveform.length() != length)
		{
			waveform.set_length(length);
			is_dirty = true;
		}

		if (is_dirty)
		{
			waveform.set_dirty();
		}
	}

} /* namespace ui */
