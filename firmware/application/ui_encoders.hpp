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
#include "ui_navigation.hpp"
#include "ui_font_fixed_8x16.hpp"
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
		std::string name;						// Encoder chip ref/name
		std::string address_symbols;			// "01", "01F"...
		std::string data_symbols;				// Same
		uint16_t clk_per_symbol;				// Oscillator periods per symbol
		uint16_t clk_per_fragment;				// Oscillator periods per symbol fragment (state)
		std::vector<std::string> bit_format;	// List of fragments for each symbol in previous *_symbols list order
		uint8_t word_length;					// Total # of symbols (not counting sync)
		std::string word_format;				// A for Address, D for Data, S for sync
		std::string sync;						// Like bit_format
		uint32_t default_frequency;				// Default encoder clk frequency (often set by shitty resistor)
		uint8_t repeat_min;						// Minimum repeat count
		uint16_t pause_symbols;					// Length of pause between repeats in symbols
	};

	const encoder_def_t encoder_defs[ENC_TYPES_COUNT] = {
		// PT2260-R2
		{
			"2260-R2",
			"01F", "01",
			1024, 128,
			{ "10001000", "11101110", "10001110" },
			12,	"AAAAAAAAAADDS",
			"10000000000000000000000000000000",
			150000,	2,
			0
		},
		
		// PT2260-R4
		{
			"2260-R4",
			"01F", "01",
			1024, 128,
			{ "10001000", "11101110", "10001110" },
			12,	"AAAAAAAADDDDS",
			"10000000000000000000000000000000",
			150000,	2,
			0
		},
		
		// PT2262
		{
			"2262   ",
			"01F", "01F",
			32, 4,
			{ "10001000", "11101110", "10001110" },
			12,	"AAAAAAAAAAAAS",
			"10000000000000000000000000000000",
			20000,	4,
			0
		},
		
		// 16-bit ?
		{
			"16-bit ",
			"01", "01",
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
			"01", "01",
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
			"01", "01",
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
			"01", "01",
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
			"0123", "0123",
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
			"01", "01",
			96, 32,
			{ "011", "001" },
			12,	"SAAAAAAAAAAAA",
			"1",
			100000,	4,
			0	// ?
		},
		
		// UM3758
		{
			"UM3758 ",
			"01F", "01",
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
			"01", "01",
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
			"01F", "01",
			16, 1,
			{ "0111111101111111", "0100000001000000", "0111111101000000" },
			9,	"SAAAAADDDD",
			"000000000000000000",
			455000,	2,
			2
		},
		
		// HT6*** TODO: Add individual variations
		{
			"HT6*** ",
			"01F", "01",
			198, 33,
			{ "011011", "001001", "001011" },
			18,	"SAAAAAAAAAAAADDDDDD",
			"0000000000000000000000000000000000001011001011001",
			80000,	3,
			10	// ?
		},
		
		// TC9148
		{
			"TC9148 ",
			"01", "01",
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
		.background = Color::black(),
		.foreground = Color::green(),
	};
	const Style style_cancel {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::red(),
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
	
	Text text_enctype {
		{ 1 * 8, 24, 5 * 8, 16 },
		"Type:"
	};
	OptionsField options_enctype {		// Options are loaded at runtime
		{ 6 * 8, 24 },
		7,
		{
		}
	};
	
	Text text_clk {
		{ 16 * 8, 3 * 8, 4 * 8, 16 },
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
		{ 16 * 8, 5 * 8, 4 * 8, 16 },
		"Bit:"
	};
	NumberField numberfield_bitduration {
		{ 21 * 8, 5 * 8 },
		4,
		{ 50, 9999 },
		1,
		' '
	};
	Text text_us1 {
		{ 25 * 8, 5 * 8, 2 * 8, 16 },
		"us"
	};
	
	Text text_wordduration {
		{ 15 * 8, 7 * 8, 5 * 8, 16 },
		"Word:"
	};
	NumberField numberfield_wordduration {
		{ 21 * 8, 7 * 8 },
		5,
		{ 300, 99999 },
		100,
		' '
	};
	Text text_us2 {
		{ 26 * 8, 7 * 8, 2 * 8, 16 },
		"us"
	};
	
	Text text_symfield {
		{ 2 * 8, 9 * 8, 5 * 8, 16 },
		"Word:"
	};
	SymField symfield_word {
		{ 2 * 8, 11 * 8 },
		20
	};
	Text text_format {
		{ 2 * 8, 13 * 8, 24 * 8, 16 },
		""
	};
	
	//Text text_format_a;	// DEBUG
	//Text text_format_d;	// DEBUG
	
	Text text_waveform {
		{ 1 * 8, 136, 9 * 8, 16 },
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
