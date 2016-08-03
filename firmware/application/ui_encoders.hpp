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
#include "ui_widget.hpp"
#include "ui_textentry.hpp"
#include "message.hpp"
#include "transmitter_model.hpp"

namespace ui {

class EncodersView : public View {
public:
	EncodersView(NavigationView& nav);
	~EncodersView();
	
	void focus() override;
	void on_show() override;
	void paint(Painter& painter) override;
	
	std::string title() const override { return "Encoders TX"; };

private:
	struct encoder_def_t {
		uint8_t bit_states;
		uint16_t clk_per_bit;
		uint16_t clk_per_symbol;
		std::vector<std::string> bit_format;
		uint8_t word_length;
		std::string format;
		bool sync_bit;
		uint32_t default_frequency;
		uint8_t repeat_min;
	};

	// P = Prelude
	// S = Sync
	// 0~9 A~J = Address/data bits
	const encoder_def_t encoder_defs[7] = {
		// PT2260
		{ 3,	1024,	128,
			{ "10001000", "11101110", "10001110" },
			12,	"0123456789ABS",
			true,	200000,	1	},
		
		// PT2262
		{ 3,	32,		4,
			{ "10001000", "11101110", "10001110" },
			12,	"0123456789ABS",
			true,	30000,	4	},
		
		// HK526E
		{ 2,	24,		8,
			{ "110", "100" },
			12,	"0123456789AB",
			false,	20000,	4	},
		
		// HT12E
		{ 2,	3,		1,
			{ "011", "001" },
			12,	"PS0123456789AB",
			true,	3000,	4	},
			
		// VD5026 13 bits ?
		{ 4,	128,	8,
			{ "1000000010000000", "1111111011111110", "1111111010000000", "1000000011111110" },
			12,	"S0123456789AB",
			true,	100000,	4	},
			
		// UM3750
		{ 2,	96,		32,
			{ "011", "001" },
			12,	"S0123456789AB",
			true,	100000,	4	},
		
		// UM3758
		{ 3,	96,		16,
			{ "011011", "001001", "011001" },
			18,	"S0123456789ABCDEFGH",
			true,	160000,	4	}
	};

	enum tx_modes {
		IDLE = 0,
		SINGLE,
		SCAN
	};
	
	uint8_t enc_type = 0;
	const encoder_def_t * encoder_def = &encoder_defs[0];
	tx_modes tx_mode = IDLE;
	//bool abort_scan = false;
	//uint8_t scan_count;
	//double scan_progress;
	//unsigned int scan_index;
	std::string debug_text = "0";
	//rf::Frequency f;
	//uint8_t repeat_index;
	
	void on_bitfield();
	void on_type_change(size_t index);
	void generate_frame();
	void update_progress();
	void start_tx(const bool scan);
	void on_txdone(int n);
	
	radio::Configuration ook_radio_config = {
		0,
		2280000,
		2500000,	// ?
		rf::Direction::Transmit,
		true,
		0,
		0,
		1,
	};
	
	const Style style_val {
		.font = font::fixed_8x16,
		.background = Color::green(),
		.foreground = Color::black(),
	};
	const Style style_cancel {
		.font = font::fixed_8x16,
		.background = Color::red(),
		.foreground = Color::black(),
	};
	
	std::array<NumberField, 18> bitfields;
	
	Text text_enctype {
		{ 2 * 8, 3 * 8, 8 * 8, 16 },
		"Encoder:"
	};
	OptionsField options_enctype {
		{ 2 * 8, 5 * 8 },
		7,
		{
			{ "PT2260 ", 0 },
			{ "PT2262 ", 1 },
			{ "HK526E ", 2 },
			{ "HT12E  ", 3 },
			{ "VD5026 ", 4 },
			{ "UM3750 ", 5 },
			{ "UM3758 ", 6 }
		}
	};
	
	Text text_clk {
		{ 15 * 8, 3 * 8, 4 * 8, 16 },
		"Clk:"
	};
	NumberField numberfield_clk {
		{ 21 * 8, 3 * 8 },
		3,
		{ 1, 300 },
		1,
		' '
	};
	Text text_kHz {
		{ 24 * 8, 3 * 8, 3 * 8, 16 },
		"kHz"
	};
	
	Text text_bitduration {
		{ 15 * 8, 5 * 8, 4 * 8, 16 },
		"Bit:"
	};
	NumberField numberfield_bitduration {
		{ 20 * 8, 5 * 8 },
		4,
		{ 300, 9999 },
		1,
		' '
	};
	Text text_us1 {
		{ 24 * 8, 5 * 8, 2 * 8, 16 },
		"us"
	};
	
	Text text_wordduration {
		{ 14 * 8, 7 * 8, 5 * 8, 16 },
		"Word:"
	};
	NumberField numberfield_wordduration {
		{ 20 * 8, 7 * 8 },
		5,
		{ 300, 99999 },
		100,
		' '
	};
	Text text_us2 {
		{ 25 * 8, 7 * 8, 2 * 8, 16 },
		"us"
	};
	
	Text text_bitfield {
		{ 2 * 8, 8 * 8, 5 * 8, 16 },
		"Word:"
	};
	
	Text text_waveform {
		{ 1 * 8, 112, 9 * 8, 16 },
		"Waveform:"
	};
	
	Text text_status {
		{ 2 * 8, 224, 128, 16 },
		"Ready"
	};
	ProgressBar progress {
		{ 16, 224 + 20, 208, 16 }
	};
	
	Button button_transmit {
		{ 16, 270, 80, 32 },
		"TX"
	};
	
	MessageHandlerRegistration message_handler_tx_done {
		Message::ID::TXDone,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const TXDoneMessage*>(p);
			this->on_txdone(message.n);
		}
	};
};

} /* namespace ui */
