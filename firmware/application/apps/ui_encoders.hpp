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

#include "ui.hpp"
#include "ui_transmitter.hpp"
#include "transmitter_model.hpp"
#include "encoders.hpp"
#include "de_bruijn.hpp"

using namespace encoders;

namespace ui
{
	class EncodersView : public View
	{
	public:
		EncodersView(NavigationView &nav);
		~EncodersView();

		void focus() override;
		std::string title() const override { return "OOK transmit"; };

	private:
		NavigationView &nav_;

		enum tx_modes
		{
			TX_MODE_IDLE = 0,
			TX_MODE_SINGLE = 1,
			TX_MODE_DEBRUIJN = 2
		};

		void update_progress();
		void generate_frame(const bool is_debruijn, const uint32_t debruijn_bits);

		std::string frame_fragments = "0";

		uint32_t samples_per_bit();
		uint16_t repeat_skip_bits_count();
		const encoder_def_t *encoder_def{};

		uint8_t scan_index;
		uint8_t scan_count;
		double scan_progress;
		uint8_t bits_per_packet; // Euquiq: the number of bits needed from de_bruijn, depends on the encoder's needs

		tx_modes tx_mode = TX_MODE_IDLE;
		uint8_t repeat_min{0};
		uint8_t repeat_index{0};
		bool abort_scan = false;

		uint8_t afsk_repeats;
		de_bruijn debruijn_seq;

		int16_t waveform_buffer[550];
		uint8_t enc_type = 0;
		char str[16];

		void draw_waveform();
		void on_bitfield();
		void on_type_change(size_t index);
		void on_tx_method_change(int32_t selected_tx_mode);
		void start_single_tx();
		void start_debruijn_tx();
		void on_tx_progress(const uint32_t progress, const bool done);

		Rect view_rect = {0, 4 * 8, 240, 168};

		Labels labels{
			{{1 * 8, 0}, "Type:", Color::light_grey()},
			{{1 * 8, 2 * 8}, "TX:", Color::light_grey()},
			{{16 * 8, 0}, "Clk:", Color::light_grey()},
			{{24 * 8, 0}, "kHz", Color::light_grey()},
			{{14 * 8, 2 * 8}, "Frame:", Color::light_grey()},
			{{26 * 8, 2 * 8}, "us", Color::light_grey()},
			{{1 * 8, 4 * 8}, "Symbols:", Color::light_grey()},
			{{1 * 8, 20 * 8}, "Waveform:", Color::light_grey()}};

		OptionsField options_enctype{
			{7 * 8, 0},
			7,
			{
				// Options are loaded at runtime
			}};

		OptionsField options_txmethod{
			{5 * 8, 2 * 8},
			8,
			{
				{"Single", TX_MODE_SINGLE},
				{"DeBruijn", TX_MODE_DEBRUIJN},
			}};

		NumberField field_clk{
			{21 * 8, 0},
			3,
			{1, 500},
			1,
			' '};

		NumberField field_frameduration{
			{21 * 8, 2 * 8},
			5,
			{300, 99999},
			100,
			' '};

		SymField symfield_word{
			{1 * 8, 6 * 8},
			20,
			SymField::SYMFIELD_DEF};

		Text text_format{
			{1 * 8, 8 * 8, 25 * 8, 16},
			""};

		Waveform waveform{
			{0, 10 * 16, 240, 32},
			waveform_buffer,
			0,
			0,
			true,
			Color::yellow()};

		Text text_status{
			{1 * 8, 13 * 16, 128, 16},
			"Ready"};

		ProgressBar progressbar{
			{1 * 8, 13 * 16 + 20, 224, 16}};

		TransmitterView tx_view{
			16 * 16,
			50000,
			9};

		MessageHandlerRegistration message_handler_tx_progress{
			Message::ID::TXProgress,
			[this](const Message *const p)
			{
				const auto message = *reinterpret_cast<const TXProgressMessage *>(p);
				this->on_tx_progress(message.progress, message.done);
			}};
	};

} /* namespace ui */
