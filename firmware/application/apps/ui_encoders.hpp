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

		static constexpr Style style_success{
			.font = font::fixed_8x16,
			.background = Color::black(),
			.foreground = Color::green(),
		};
		static constexpr Style style_info{
			.font = font::fixed_8x16,
			.background = Color::black(),
			.foreground = Color::green(),
		};
		static constexpr Style style_err{
			.font = font::fixed_8x16,
			.background = Color::black(),
			.foreground = Color::red(),
		};

		enum tx_modes
		{
			TX_MODE_IDLE = 0,
			TX_MODE_MANUAL = 1,
			TX_MODE_DEBRUIJN = 2,
			TX_MODE_BRUTEFORCE = 3
		};

		void init_progress();
		void update_progress();
		void generate_frame();
		const char *get_symbols_bit_fragments(const uint32_t index);

		std::string frame_fragments = "0";

		uint32_t samples_per_bit();
		uint16_t repeat_skip_bits_count();
		const encoder_def_t *encoder_def{};

		tx_modes tx_mode = TX_MODE_IDLE;
		uint8_t tx_repeat_min{0};
		uint8_t tx_repeat_index{0};
		uint8_t afsk_repeats;

		int16_t waveform_buffer[550];
		uint8_t enc_type = 0;
		char str[16];

		// debruijn
		de_bruijn debruijn_seq;
		uint8_t debruijn_index;
		uint8_t debruijn_max;
		uint32_t debruijn_bits;
		uint8_t debruijn_bits_per_packet; // Euquiq: the number of bits needed from de_bruijn, depends on the encoder's needs

		// bruteforce
		uint32_t bruteforce_index = 0;
		uint32_t bruteforce_max = 0;

		void on_tx_progress(const uint32_t progress, const bool done);

		void draw_waveform();
		void reset_symfield();
		void check_if_encoder_is_vuln_to_debruijn();
		void check_if_encoder_can_be_reversed();

		void start_single_tx();
		void start_debruijn_tx();
		void tick_debruijn_tx();
		void start_bruteforce_tx();
		void tick_bruteforce_tx();
		void tx();
		void stop_tx();

		Rect view_rect = {0, 4 * 8, 240, 168};

		Labels labels{
			{{1 * 8, 0}, "Type:", Color::light_grey()},
			{{1 * 8, 2 * 8}, "TX:", Color::light_grey()},
			{{16 * 8, 0}, "Clk:", Color::light_grey()},
			{{26 * 8, 0}, "kHz", Color::light_grey()},
			{{16 * 8, 2 * 8}, "Frame:", Color::light_grey()},
			{{1 * 8, 4 * 8}, "Repeat:", Color::light_grey()},
			{{28 * 8, 2 * 8}, "us", Color::light_grey()},
			{{1 * 8, 6 * 8}, "Symbols:", Color::light_grey()},
			{{1 * 8, 18 * 8}, "Waveform:", Color::light_grey()}};

		Checkbox checkbox_reversed{
			{20 * 8, 18 * 8},
			12,
			"Reverse",
			true};

		OptionsField options_encoder{
			{7 * 8, 0},
			7,
			{
				// Options are loaded at runtime
			}};

		OptionsField options_tx_method{
			{5 * 8, 2 * 8},
			10,
			{
				{"Manual", TX_MODE_MANUAL},
				{"Bruteforce", TX_MODE_BRUTEFORCE},
				{"DeBruijn", TX_MODE_DEBRUIJN},
			}};

		NumberField field_clk{
			{23 * 8, 0},
			3,
			{1, 500},
			1,
			' '};

		NumberField field_frameduration{
			{23 * 8, 2 * 8},
			5,
			{300, 99999},
			100,
			' '};

		NumberField field_repeat_min{
			{8 * 8, 4 * 8},
			5,
			{1, 100},
			1,
			' '};

		SymField symfield_word{
			{1 * 8, 8 * 8},
			20,
			SymField::SYMFIELD_DEF};

		Text text_format{
			{1 * 8, 10 * 8, 25 * 8, 16},
			""};

		Waveform waveform{
			{0, 21 * 8, 240, 32},
			waveform_buffer,
			0,
			0,
			true,
			Color::yellow()};

		Text text_progress{
			{1 * 8, 13 * 16, 224, 16},
			"Ready"};

		ProgressBar progress_bar{
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
