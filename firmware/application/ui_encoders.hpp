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

#define ENC_TYPES_COUNT 14

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
		std::string name;
		uint8_t address_bit_states;		// Often 0, 1, [f, [4]]
		uint8_t data_bit_states;		// Often 0, 1
		uint16_t clk_per_bit;			// Oscillator periods per bit
		uint16_t clk_per_symbol;		// Oscillator periods per bit fragment
		std::vector<std::string> bit_format;
		uint8_t word_length;			// Total # of bits
		std::string word_format;
		std::string sync;				// Like bit_format
		uint32_t default_frequency;		// Default encoder clk frequency
		uint8_t repeat_min;
		uint32_t pause_symbols;
	};

	// S = Sync
	// A/D = Address/data bits
	const encoder_def_t encoder_defs[ENC_TYPES_COUNT] = {
		// PT2260-R2
		{
			"2260-R2",
			3, 2,
			1024, 128,
			{ "10001000", "11101110", "10001110" },
			12,	"AAAAAAAAAADDS",
			"10000000000000000000000000000000",
			200000,	1,
			10	// ?
		},
		
		// PT2260-R4
		{
			"2260-R4",
			3, 2,
			1024, 128,
			{ "10001000", "11101110", "10001110" },
			12,	"AAAAAAAADDDDS",
			"10000000000000000000000000000000",
			200000,	1,
			10	// ?
		},
		
		// PT2262
		{
			"2262   ",
			3, 2,
			1024, 128,
			{ "10001000", "11101110", "10001110" },
			12,	"AAAAAAAAAAAAS",
			"10000000000000000000000000000000",
			200000,	1,
			10	// ?
		},
		
		// 16-bit ?
		{
			"16-bit ",
			2, 2,
			32, 8,
			{ "1110", "1000" },		// Opposite ?
			16,	"AAAAAAAAAAAAAAAAS",
			"100000000000000000000",
			25000,	50,
			0	// ?
		},
		
		// RT1527
		{
			"1527   ",
			2, 2,
			128, 32,
			{ "1000", "1110" },
			24,	"SAAAAAAAAAAAAAAAAAAAADDDD",
			"10000000000000000000000000000000",
			100000,	4,
			10	// ?
		},
		
		// HK526E
		{
			"526    ",
			2, 2,
			24, 8,
			{ "110", "100" },
			12,	"AAAAAAAAAAAA",
			"",
			20000, 4,
			10	// ?
		},
		
		// HT12E
		{
			"12E    ",
			2, 2,
			3, 1,
			{ "011", "001" },
			12,	"SAAAAAAAADDDD",
			"0000000000000000000000000000000000001",
			3000, 4,
			10	// ?
		},
			
		// VD5026 13 bits ?
		{
			"5026   ",
			4, 4,
			128, 8,
			{ "1000000010000000", "1111111011111110", "1111111010000000", "1000000011111110" },
			12,	"SAAAAAAAAAAAA",
			"000000000000000000000000000000000000000000000001",		// ?
			100000,	4,
			10	// ?
		},
		
		// UM3750
		{
			"UM3750 ",
			2, 2,
			96, 32,
			{ "011", "001" },
			12,	"SAAAAAAAAAAAA",
			"1",
			100000,	4,
			10	// ?
		},
		
		// UM3758
		{
			"UM3758 ",
			3, 2,
			96, 16,
			{ "011011", "001001", "011001" },
			18,	"SAAAAAAAAAADDDDDDDD",
			"1",
			160000,	4,
			10	// ?
		},
		
		// BA5104
		{
			"BA5104 ",
			2, 2,
			3072, 768,
			{ "1000", "1110" },
			9,	"SDDAAAAAAA",
			"",
			455000,	4,
			10	// ?
		},
			
		// MC145026
		{
			"145026 ",
			3, 2,
			16, 1,
			{ "0111111101111111", "0100000001000000", "0111111101000000" },
			9,	"SAAAAADDDD",
			"000000000000000000",
			455000,	2,
			10	// ?
		},
		
		// HT6*** TODO: Add individual variations
		{
			"HT6*** ",
			3, 2,
			198, 33,
			{ "011011", "001001", "001011" },
			18,	"SAAAAAAAAAAAADDDDDD",
			"0000000000000000000000000000000000001011001011001",
			100000,	3,
			10	// ?
		},
		
		// TC9148
		{
			"TC9148 ",
			2, 2,
			48, 12,
			{ "1000", "1110", },
			12,	"AAAAAAAAAAAA",
			"",
			455000,	3,
			10	// ?
		}
	};

	enum tx_modes {
		IDLE = 0,
		SINGLE,
		SCAN
	};
	
	Painter * painter_;
	uint8_t enc_type = 0;
	const encoder_def_t * encoder_def;
	tx_modes tx_mode = IDLE;
	//bool abort_scan = false;
	//uint8_t scan_count;
	//double scan_progress;
	//unsigned int scan_index;
	std::string debug_text = "0";
	//rf::Frequency f;
	uint8_t repeat_index;
	
	void draw_waveform();
	void on_bitfield();
	void on_type_change(size_t index);
	void generate_frame();
	void update_progress();
	void start_tx(const bool scan);
	void on_txdone(int n);
	
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
	
	const Style style_address {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::red(),
	};
	const Style style_data {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::blue(),
	};
	
	SymField bitfield {
		{ 16 + 8, 80 },
		12,
		{ 0, 1 }
	};
	
	Text text_enctype {
		{ 2 * 8, 3 * 8, 8 * 8, 16 },
		"Encoder:"
	};
	OptionsField options_enctype {
		{ 2 * 8, 5 * 8 },
		7,
		{
		}
	};
	
	Text text_clk {
		{ 15 * 8, 3 * 8, 4 * 8, 16 },
		"Clk:"
	};
	NumberField numberfield_clk {
		{ 21 * 8, 3 * 8 },
		3,
		{ 1, 500 },
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
		{ 50, 9999 },
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
	//Text text_format_a;	// DEBUG
	//Text text_format_d;	// DEBUG
	
	Text text_waveform {
		{ 1 * 8, 16 * 8, 9 * 8, 16 },
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
