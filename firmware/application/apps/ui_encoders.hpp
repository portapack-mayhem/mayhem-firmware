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

namespace ui {

class EncodersView : public View {
public:
	EncodersView(NavigationView& nav);
	~EncodersView();
	

	EncodersView(NavigationView& nav, Rect parent_rect);
	
	EncodersView(const EncodersView&) = delete;
	EncodersView(EncodersView&&) = delete;
	EncodersView& operator=(const EncodersView&) = delete;
	EncodersView& operator=(EncodersView&&) = delete;

	void focus() override;
	
	std::string title() const override { return "OOK transmit"; };




private:
	NavigationView& nav_;

	enum tx_modes {
		IDLE = 0,
		SINGLE,
		SCAN
	};

	uint32_t samples_per_bit();
	uint32_t pause_symbols();
	void generate_frame(bool is_debruijn, uint32_t debruijn_bits);
	
	std::string frame_fragments = "0";
	const encoder_def_t * encoder_def { };
	uint8_t bits_per_packet; 	//Euquiq: the number of bits needed from de_bruijn, depends on the encoder's needs
	
	tx_modes tx_mode = IDLE;
	uint8_t repeat_index { 0 };
	uint32_t scan_count;
	uint32_t scan_index;
	double scan_progress;
	bool abort_scan = false;
	char str[16];

	uint8_t afsk_repeats;
	de_bruijn debruijn_seq;
	
	void update_progress();
	void start_tx(const bool scan);
	void on_tx_progress(const uint32_t progress, const bool done);

	int16_t waveform_buffer[550];
	
	uint8_t enc_type = 0;

	void draw_waveform();
	void on_bitfield();
	void on_type_change(size_t index);

	Labels labels {
		{ { 1 * 8, 1 * 8}, "Type:", Color::light_grey() },
		{ { 16 * 8, 1 * 8}, "Clk:", Color::light_grey() },
		{ { 24 * 8, 1 * 8}, "kHz", Color::light_grey() },
		{ { 14 * 8, 3 * 8 }, "Frame:", Color::light_grey() },
		{ { 26 * 8, 3 * 8 }, "us", Color::light_grey() },
		{ { 2 * 8, 5 * 8 }, "Symbols:", Color::light_grey() },
		{ { 1 * 8, 13 * 8 }, "Waveform:", Color::light_grey() }
	};

	OptionsField options_enctype {		// Options are loaded at runtime
		{ 6 * 8, 1 * 8 },
		7,
		{
		}
	};

	NumberField field_clk {
		{ 21 * 8, 1 * 8 },
		3,
		{ 1, 500 },
		1,
		' '
	};

	NumberField field_frameduration {
		{ 21 * 8, 3 * 8 },
		5,
		{ 300, 99999 },
		100,
		' '
	};
	
	SymField symfield_word {
		{ 2 * 8, 7 * 8 },
		20,
		SymField::SYMFIELD_DEF
	};
	
	Text text_format {
		{ 2 * 8, 9 * 8, 24 * 8, 16 },
		""
	};
	
	Waveform waveform {
		{ 0, 15 * 8, 240, 32 },
		waveform_buffer,
		0,
		0,
		true,
		Color::yellow()
	};	

	Text text_status {
		{ 2 * 8, 13 * 16, 128, 16 },
		"Ready"
	};

	Button button_scan {
		{ 8 * 8, 11 * 16, 120, 28 },
		"DE BRUIJN TX"
	};
	
	ProgressBar progressbar {
		{ 2 * 8, 13 * 16 + 20, 208, 16 }
	};
	
	TransmitterView tx_view {
		16 * 16,
		50000,
		9
	};
	
	MessageHandlerRegistration message_handler_tx_progress {
		Message::ID::TXProgress,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
			this->on_tx_progress(message.progress, message.done);
		}
	};
};

} /* namespace ui */
