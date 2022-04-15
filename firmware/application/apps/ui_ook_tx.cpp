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
			&field_pause_between_symbols,
			&symfield_word,
			&text_format,
		});

		options_encoder.on_change = [this](size_t index, int32_t)
		{
			encoder_def = &encoder_defs[index];

			field_clk.set_value(encoder_def->default_clk_speed / 1000);
			field_repeat_min.set_value(encoder_def->repeat_min);
			options_period_per_symbol.set_by_value(encoder_def->period_per_symbol);
			field_pause_between_symbols.set_value(encoder_def->pause_symbols);

			reset_symfield();
			check_if_encoder_is_vuln_to_debruijn();
			check_if_encoder_can_be_reversed();

			// reset the debruijn sequencer in case the encoder is vulnerable
			if (encoder_def->is_vuln_to_debruijn)
			{
				reset_debruijn();
			}

			if (on_encoder_change)
				on_encoder_change();

			if (on_waveform_change_request)
				on_waveform_change_request();
		};

		options_tx_method.on_change = [this](size_t, int32_t value)
		{
			if (value == TX_MODE_DEBRUIJN)
			{
				// Set the repeat to 0
				field_repeat_min.set_value(0);
			}
			else
			{
				// set to the default repeat min at the selected encoder
				field_repeat_min.set_value(encoder_def->repeat_min);
			}

			reset_symfield();
			check_if_encoder_is_vuln_to_debruijn();

			if (on_waveform_change_request)
				on_waveform_change_request();
		};

		symfield_word.on_change = [this]()
		{
			if (on_waveform_change_request)
				on_waveform_change_request();
		};

		// Selecting input clock changes symbol and word duration
		field_clk.on_change = [this](int32_t value)
		{
			// value is in kHz, new_value is in us
			int32_t new_value = (options_period_per_symbol.selected_index_value() * 1000000) / (value * 1000);
			if (new_value != field_frameduration.value())
				field_frameduration.set_value(new_value * encoder_def->word_length, false);
		};

		// Selecting word duration changes input clock and symbol duration
		field_frameduration.on_change = [this](int32_t value)
		{
			// value is in us, new_value is in kHz
			int32_t new_value = (value * 1000) / (encoder_def->word_length * options_period_per_symbol.selected_index_value());
			if (new_value != field_clk.value())
				field_clk.set_value(1000000 / new_value, false);
		};

		options_period_per_symbol.on_change = [this](size_t, int32_t)
		{
			// trigger the change on both fields
			field_clk.on_change(field_clk.value());
			field_frameduration.on_change(field_frameduration.value());
		};

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

	void OOKTxGeneratorView::check_if_encoder_is_vuln_to_debruijn()
	{
		// if the selected tx method is DEBRUIJN, check if the encoder is vulnerable to DEBRUIJN
		if (options_tx_method.selected_index_value() == TX_MODE_DEBRUIJN && !encoder_def->is_vuln_to_debruijn)
		{
			if (on_status_change)
				on_status_change("Not vuln to DeBruijn");
			return;
		}

		if (on_status_change)
			on_status_change("");
	}

	void OOKTxGeneratorView::check_if_encoder_can_be_reversed()
	// TODO: need to review this piece
	{
		// if the selected tx method is DEBRUIJN, check if the encoder is vulnerable to DEBRUIJN
		if (sizeof(encoder_def->symbols_bit_fragments) == 2)
		{
			// checkbox_reversed.set_focusable(true);
		}
		else
		{
			// checkbox_reversed.set_focusable(false);
			// checkbox_reversed.set_value(false);
		}
	}

	void OOKTxGeneratorView::reset_symfield()
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

	void OOKTxGeneratorView::reset_debruijn()
	{
		if (
			debruijn_sequencer.k == 2 &&
			debruijn_sequencer.n == encoder_def->word_length &&
			debruijn_sequencer.sequence.length() > 0)
			return;

		debruijn_sequencer.init("01", encoder_def->word_length, 32);

		// TODO: we need to stream the debruijn sequence instead of generating it all at once!
		// disabled in the encoder view as it just screws up on 16bits
		// debruijn_sequencer.generate();
	}

	const char *OOKTxGeneratorView::get_symbols_bit_fragments(const uint8_t index, const bool reversed)
	{
		if (reversed)
		{
			return encoder_def->symbols_bit_fragments[(index == 0) ? 1 : 0];
		}

		return encoder_def->symbols_bit_fragments[index];
	}

	uint32_t OOKTxGeneratorView::get_repeat_total()
	{
		switch (options_tx_method.selected_index_value())
		{
		case TX_MODE_MANUAL:
		case TX_MODE_BRUTEFORCE:
			return field_repeat_min.value();

		case TX_MODE_DEBRUIJN:
			return 1;
		}

		return 1;
	}

	uint32_t OOKTxGeneratorView::get_frame_part_total()
	{
		switch (options_tx_method.selected_index_value())
		{
		case TX_MODE_BRUTEFORCE:
			return symfield_word.get_possibilities_count();

		case TX_MODE_DEBRUIJN: // check the size of the whole debruijn sequence
			return debruijn_sequencer.get_total_parts();

		case TX_MODE_MANUAL:
			return 1;
		}

		return 1;
	}

	std::string OOKTxGeneratorView::generate_frame_part(const uint32_t frame_part_index, const bool reversed)
	{
		int32_t mode = options_tx_method.selected_index_value();

		// NOTE: bruteforce might need to be check to enforce the frame_part_index
		// TODO: :this:

		if (mode == TX_MODE_MANUAL || mode == TX_MODE_BRUTEFORCE)
		{
			std::string frame_fragments = "";

			for (uint8_t i = 0; i < encoder_def->word_length; i++)
				if (encoder_def->word_format[i] == 'S')
					frame_fragments += encoder_def->sync_bit_fragment;
				else
					frame_fragments += get_symbols_bit_fragments(symfield_word.get_sym(i), reversed);

			return frame_fragments;
		}

		if (mode == TX_MODE_DEBRUIJN)
		{
			if (!encoder_def->is_vuln_to_debruijn)
				return "0";

			std::string frame_fragments = "";
			std::string frame_part = debruijn_sequencer.get_part(frame_part_index);

			for (uint8_t i = 0; i < frame_part.length(); i++)
			{
				frame_fragments += get_symbols_bit_fragments(("1" == &frame_part[i]) ? 1 : 0, reversed);
			}

			return frame_fragments;
		}

		return "0";
	}

	uint32_t OOKTxGeneratorView::samples_per_bit()
	{
		return OOK_SAMPLERATE / ((field_clk.value() * 1000) / (options_period_per_symbol.selected_index_value() / encoder_def->bit_fragments_length_per_symbol));
	}

	// 	///////////////////////////////////////////////////////////////////////////////
	// 	// OOKTxFilesView

	OOKTxFilesView::OOKTxFilesView(
		NavigationView &, Rect parent_rect)
	{
		set_parent_rect(parent_rect);
		hidden(true);

		add_children({
			&labels,
		});
	}

	void OOKTxFilesView::focus()
	{
		// field_debug.focus();
	}

	uint32_t OOKTxFilesView::get_repeat_total() { return 1; }
	uint32_t OOKTxFilesView::get_frame_part_total() { return 1; }

	std::string OOKTxFilesView::generate_frame_part(const uint32_t frame_part_index, const bool reversed)
	{
		return "0";
	}

	void OOKTxFilesView::refresh_list()
	{
		// auto reader = std::make_unique<FileReader>();

		// file_list.clear();
		// c_page = page;

		// // List directories and files, put directories up top
		// uint32_t count = 0;
		// for (const auto &entry : std::filesystem::directory_iterator(u"OOK", u"*"))
		// {
		// 	if (std::filesystem::is_regular_file(entry.status()))
		// 	{
		// 		if (entry.path().string().length())
		// 		{

		// 			auto entry_extension = entry.path().extension().string();

		// 			for (auto &c : entry_extension)
		// 				c = toupper(c);

		// 			if (entry_extension == ".OOK")
		// 			{

		// 				if (reader->open(u"/WAV/" + entry.path().native()))
		// 				{
		// 					if ((reader->channels() == 1) && (reader->bits_per_sample() == 8))
		// 					{
		// 						// sounds[c].ms_duration = reader->ms_duration();
		// 						// sounds[c].path = u"WAV/" + entry.path().native();
		// 						if (count >= (page - 1) * 100 && count < page * 100)
		// 						{
		// 							file_list.push_back(entry.path());
		// 							if (file_list.size() == 100)
		// 							{
		// 								page++;
		// 								break;
		// 							}
		// 						}
		// 						count++;
		// 					}
		// 				}
		// 			}
		// 		}
		// 	}
		// }

		// if (!file_list.size())
		// {
		// 	// Hide widgets, show warning
		// 	if (page == 1)
		// 	{
		// 		menu_view.hidden(true);
		// 		text_empty.hidden(false);
		// 		set_dirty();
		// 	}
		// 	else
		// 	{
		// 		page = 1;
		// 		refresh_list();
		// 		return;
		// 	}
		// }
		// else
		// {
		// 	// Hide warning, show widgets
		// 	menu_view.hidden(false);
		// 	text_empty.hidden(true);
		// 	set_dirty();

		// 	menu_view.clear();

		// 	for (size_t n = 0; n < file_list.size(); n++)
		// 	{
		// 		menu_view.add_item({file_list[n].string().substr(0, 30),
		// 							ui::Color::white(),
		// 							nullptr,
		// 							[this]()
		// 							{
		// 								on_select_entry();
		// 							}});
		// 	}

		// 	page_info.set("Page: " + to_string_dec_uint(c_page) + "    Sounds: " + to_string_dec_uint(file_list.size()));
		// 	menu_view.set_highlighted(0); // Refresh
		// }

		// if (file_list.size() < 100)
		// {
		// 	page = 1;
		// }
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
			&field_wordlength,
			&field_fragments,
			&field_clk,
			&field_frameduration,
			&options_period_per_symbol,
			&field_pause_between_symbols,
			&symfield_fragment_0,
			&symfield_fragment_1,
		});

		field_wordlength.on_change = [this](uint32_t)
		{
			reset_debruijn();

			if (on_waveform_change_request)
				on_waveform_change_request();
		};

		field_fragments.on_change = [this](uint32_t)
		{
			reset_symfield();

			if (on_waveform_change_request)
				on_waveform_change_request();
		};

		symfield_fragment_0.on_change = [this]()
		{
			if (on_waveform_change_request)
				on_waveform_change_request();
		};
		symfield_fragment_1.on_change = [this]()
		{
			if (on_waveform_change_request)
				on_waveform_change_request();
		};

		// Selecting input clock changes symbol and word duration
		field_clk.on_change = [this](int32_t value)
		{
			// value is in kHz, new_value is in us
			int32_t new_value = (options_period_per_symbol.selected_index_value() * 1000000) / (value * 1000);
			if (new_value != field_frameduration.value())
				field_frameduration.set_value(new_value * field_wordlength.value(), false);
		};

		// Selecting word duration changes input clock and symbol duration
		field_frameduration.on_change = [this](int32_t value)
		{
			// value is in us, new_value is in kHz
			int32_t new_value = (value * 1000) / (field_wordlength.value() * options_period_per_symbol.selected_index_value());
			if (new_value != field_clk.value())
				field_clk.set_value(1000000 / new_value, false);
		};

		options_period_per_symbol.on_change = [this](size_t, int32_t)
		{
			// trigger the change on both fields
			field_clk.on_change(field_clk.value());
			field_frameduration.on_change(field_frameduration.value());
		};

		// set default values
		field_wordlength.set_value(4);
		field_fragments.set_value(4);
		field_clk.set_value(250);
		options_period_per_symbol.set_by_value(10);
		symfield_fragment_0.set_next_possibility();

		for (uint32_t i = 0; i < field_fragments.value(); i++)
			symfield_fragment_1.set_sym(i, 1);
	}

	void OOKTxDeBruijnView::focus()
	{
		field_wordlength.focus();
	}

	uint32_t OOKTxDeBruijnView::samples_per_bit()
	{
		return OOK_SAMPLERATE / ((field_clk.value() * 1000) / (options_period_per_symbol.selected_index_value() / field_fragments.value()));
	}

	void OOKTxDeBruijnView::reset_symfield()
	{
		std::string format_string = "";
		uint32_t fragments_length = field_fragments.value();

		symfield_fragment_0.set_length(fragments_length);
		symfield_fragment_1.set_length(fragments_length);

		for (uint32_t i = 0; i < fragments_length; i++)
		{
			symfield_fragment_0.set_symbol_list(i, symfield_symbols);
			symfield_fragment_1.set_symbol_list(i, symfield_symbols);
		}
	}

	void OOKTxDeBruijnView::reset_debruijn()
	{
		uint32_t word_length = field_wordlength.value();

		if (
			debruijn_sequencer.k == 2 &&
			debruijn_sequencer.n == word_length &&
			debruijn_sequencer.sequence.length() > 0)
			return;

		debruijn_sequencer.init(symfield_symbols, word_length, 32);

		// TODO: we need to stream the debruijn sequence instead of generating it all at once!
		debruijn_sequencer.generate();
	}

	uint32_t OOKTxDeBruijnView::get_repeat_total() { return 1; }
	uint32_t OOKTxDeBruijnView::get_frame_part_total()
	{
		return debruijn_sequencer.get_total_parts();
	}

	std::string OOKTxDeBruijnView::generate_frame_part(const uint32_t frame_part_index, const bool reverse)
	// TODO: we still need to implement the reverse flag
	{
		uint32_t fragments_length = field_fragments.value();
		std::string fragment_0 = ""; // symfield_fragment_0.value_string();
		std::string fragment_1 = ""; // symfield_fragment_1.value_string();

		// build fragments
		for (uint32_t i = 0; i < fragments_length; i++)
		{
			fragment_0 += symfield_symbols[symfield_fragment_0.get_sym(i)];
			fragment_1 += symfield_symbols[symfield_fragment_1.get_sym(i)];
		}

		std::string frame_fragments = "";
		std::string frame_part = debruijn_sequencer.get_part(frame_part_index);

		for (uint32_t i = 0; i < frame_part.length(); i++)
		{
			bool bit = frame_part[i] == symfield_symbols[1];

			if (reverse)
				bit = !bit;

			frame_fragments += (bit) ? fragment_1 : fragment_0;
		}

		return frame_fragments;
	}

	// 	///////////////////////////////////////////////////////////////////////////////
	// 	// OOKTxView

	OOKTxView::OOKTxView(NavigationView &nav) : nav_{nav}
	{
		baseband::run_image(portapack::spi_flash::image_tag_ook);

		transmitter_model.set_sampling_rate(OOK_SAMPLERATE);
		transmitter_model.set_baseband_bandwidth(1750000);

		add_children({
			&labels,
			&tab_view,

			// tab views
			&view_debruijn,
			&view_files,
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
			refresh();
		};

		// View hooks

		view_files.on_waveform_change_request = [this]()
		{
			refresh();
		};
		view_generator.on_waveform_change_request = [this]()
		{
			refresh();
		};
		view_debruijn.on_waveform_change_request = [this]()
		{
			refresh();
		};

		view_debruijn.on_status_change = [this](const std::string e)
		{
			if (err != e)
			{
				err = e;
				progress_update();
			}
		};
		view_files.on_status_change = [this](const std::string e)
		{
			if (err != e)
			{
				err = e;
				progress_update();
			}
		};
		view_generator.on_status_change = [this](const std::string e)
		{
			if (err != e)
			{
				err = e;
				progress_update();
			}
		};

		view_generator.on_encoder_change = [this]()
		{
			// reset reversed checkbox
			checkbox_reversed.set_value(false);
		};

		// start on debruijn generator
		tab_view.set_selected(2);
	}

	void OOKTxView::focus()
	{
		tab_view.focus();
		refresh();
	}

	void OOKTxView::refresh()
	{
		reset_cursors();
		generate_frame_part();
		progress_update();
		draw_waveform();
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
			frame_fragments += view_files.generate_frame_part(
				tx_mode,
				checkbox_reversed.value());

		if (tab_view.selected() == 1)
			frame_fragments += view_generator.generate_frame_part(
				tx_mode,
				checkbox_reversed.value());

		if (tab_view.selected() == 2)
			frame_fragments += view_debruijn.generate_frame_part(
				tx_mode,
				checkbox_reversed.value());
	}

	// NOTE: should be called after changing the tx_mode
	void OOKTxView::progress_reset()
	{
		progress_bar.set_max(frame_parts_cursor.total * repeat_cursor.total);
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
			text_progress.set("Ready F(" + to_string_dec_uint(frame_parts_cursor.total) + ")" + "R(" + to_string_dec_uint(repeat_cursor.total) + ")");
			progress_bar.set_value(0);
		}
		else
		{
			text_progress.set_style(&style_info);
			text_progress.set(
				to_string_dec_uint(frame_parts_cursor.index + 1) + "/" + to_string_dec_uint(frame_parts_cursor.total) +
				" (" + to_string_dec_uint(repeat_cursor.index + 1) + "/" + to_string_dec_uint(repeat_cursor.total) + ")");
		}

		progress_bar.set_value(frame_parts_cursor.index * repeat_cursor.total + repeat_cursor.index);
	}

	void OOKTxView::start_tx()
	{
		switch (tab_view.selected())
		{
		// File Loader View TX
		case 0:

			// samples_per_bit = view_generator.samples_per_bit();
			// pause_between_symbols = view_generator.field_pause_between_symbols.value();

			break;

		// Generator View TX
		case 1:
			// TODO: disable access to all inputs

			view_generator.symfield_word.set_focusable(false);

			samples_per_bit = view_generator.samples_per_bit();
			pause_between_symbols = view_generator.field_pause_between_symbols.value();

			switch (view_generator.options_tx_method.selected_index_value())
			{
			case TX_MODE_MANUAL:
				tx_mode = TX_MODE_MANUAL;
				break;

			case TX_MODE_DEBRUIJN:
				if (!view_generator.encoder_def->is_vuln_to_debruijn)
				{
					stop_tx();
					view_generator.check_if_encoder_is_vuln_to_debruijn();
					return;
				}

				tx_mode = TX_MODE_DEBRUIJN;
				view_generator.reset_debruijn();
				break;

			case TX_MODE_BRUTEFORCE:
				tx_mode = TX_MODE_BRUTEFORCE;
				frame_parts_cursor.index = view_generator.symfield_word.get_possibility();
				break;
			}
			break;

		// DeBruijn View TX
		case 2:
			tx_mode = TX_MODE_DEBRUIJN;
			view_debruijn.reset_debruijn();

			// TODO: disable access to all inputs

			samples_per_bit = view_debruijn.samples_per_bit();
			pause_between_symbols = view_debruijn.field_pause_between_symbols.value();

			break;
		}

		tx_view.set_transmitting(true);
		reset_cursors();
		progress_reset();
		tx();
	}

	// on_tx_progress
	// this method captures a tx message and decides what to do next.
	// ideally, it should attempt first to repeat the frame as repetitions are
	// handled by the baseband logic and then it should attempt to send the other
	// frame parts.
	void OOKTxView::on_tx_progress(const uint32_t, const bool done)
	{
		if (tx_mode == TX_MODE_IDLE)
			return;

		// Update cursors
		repeat_cursor.index++;
		if (done)
		{
			frame_parts_cursor.index++;
		}

		// check if we have other frames to transmit or if we want to stop transmission
		if (done)
		{

			if (repeat_cursor.is_done())
			{
				if (frame_parts_cursor.is_done())
				{
					stop_tx();
					return;
				}

				// this needs to be assessed here, if we reset it before this may cause
				// the last frame to be skipped (assuming, didn't test it)
				repeat_cursor.index = 0;
			}

			if (view_generator.options_tx_method.selected_index_value() == TX_MODE_BRUTEFORCE)
			{
				view_generator.symfield_word.set_next_possibility();
			}

			// call tx
			tx();
		}
		else
		{
			// in case we're not done yet, we still want to update the progress
			progress_update();
		}
	}

	void OOKTxView::tx()
	{
		generate_frame_part();
		progress_update();

		size_t bitstream_length = make_bitstream(frame_fragments);

		transmitter_model.enable();

		baseband::set_ook_data(
			bitstream_length,
			samples_per_bit,
			repeat_cursor.total,
			pause_between_symbols);

		// we're only pushing the ui updates past the baseband comm to ensure
		// we decrease the delay on TX frame burst cases
		draw_waveform();
	}

	void OOKTxView::stop_tx()
	{
		transmitter_model.disable();
		tx_mode = TX_MODE_IDLE;
		tx_view.set_transmitting(false);
		view_generator.symfield_word.set_focusable(true);

		reset_cursors();
		progress_reset();
		generate_frame_part();
		draw_waveform();
	}

	void OOKTxView::reset_cursors()
	{
		repeat_cursor.reset();
		frame_parts_cursor.reset();

		// get the correct repeat min
		repeat_cursor.total = view_generator.field_repeat_min.value();

		// Generator
		switch (tab_view.selected())
		{
		// File Loader View TX
		case 0:
			repeat_cursor.total = view_files.get_repeat_total();
			frame_parts_cursor.total = view_files.get_frame_part_total();
			break;

		// Generator View TX
		case 1:
			repeat_cursor.total = view_generator.get_repeat_total();
			frame_parts_cursor.total = view_generator.get_frame_part_total();
			break;

		// DeBruijn TX
		case 2:
			repeat_cursor.total = view_debruijn.get_repeat_total();
			frame_parts_cursor.total = view_debruijn.get_frame_part_total();
			break;
		}
	}

	void OOKTxView::draw_waveform()
	{
		size_t length = frame_fragments.length();

		for (int16_t n = 0; n < length; n++)
			waveform_buffer[n] = (frame_fragments[n] == '0') ? 0 : 1;

		waveform.set_length(length);
		waveform.set_dirty();
	}

} /* namespace ui */
