/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include <vector>
#include <cstring>
#include <string>

#ifndef __ENCODERS_H__
#define __ENCODERS_H__

namespace encoders
{

#define ENC_TYPES_COUNT 18
#define ENCODER_UM3750 12
#define OOK_SAMPLERATE 2280000U
#define OOK_DEFAULT_STEP 8 // 70 kHz carrier frequency

	size_t make_bitstream(std::string &fragments);
	void bitstream_append(size_t &bitstream_length, uint32_t bit_count, uint32_t bits);

	struct encoder_def_t
	{
		// Encoder details
		char name[16];			  // Encoder chip ref/name
		bool is_vuln_to_debruijn; // True if the encoder is vulnerable to the debruijn attack
		uint8_t repeat_min;		  // Minimum repeat count (ignored on DeBruijn mode)

		// Word and Symbol format (Address, Data, Sync)
		uint8_t word_length;  // Total # of symbols (not counting sync)
		char word_format[32]; // A for Address, D for Data, S for sync

		// Symbol setup - Address + Data + Sync bit fragments
		char symfield_address_symbols[8];		 // List of possible symbols like "01", "01F"...
		char symfield_data_symbols[8];			 // Same as symfield_address_symbols
		uint8_t bit_fragments_length_per_symbol; // Indicates the length of the symbols_bit_fragmentss. Will be used to divide the symbol clock osc periods
		char symbols_bit_fragments[4][20];		 // List of fragments for each symbol in previous *_symbols list order
		char sync_bit_fragment[64];				 // Like symbols_bit_fragments

		// timing
		uint16_t period_per_symbol; // Oscillator periods per symbol
		uint32_t default_clk_speed; // Default encoder clk frequency (often set by shitty resistor)
		uint16_t pause_symbols;		// Length of pause between repeats in symbols
	};

	// Warning ! If this is changed, make sure that ENCODER_UM3750 is still valid !
	constexpr encoder_def_t encoder_defs[ENC_TYPES_COUNT] = {

		// generic 8-bit encoder
		{
			// Encoder details
			"8-bits", // name
			true,	  // is_vuln_to_debruijn
			50,		  // repeat_min

			// Word and Symbol format (Address, Data, Sync)
			8,			// word_length
			"AAAAAAAA", // word_format

			// Symbol setup - Address + Data + Sync bit fragments
			"01",			  // symfield_address_symbols
			"01",			  // symfield_data_symbols
			4,				  // bit_fragments_length_per_symbol
			{"1000", "1110"}, // symbols_bit_fragments
			"",				  // sync_bit_fragment

			// Speed and clocks
			32,	   // period_per_symbol
			25000, // default_clk_speed
			0,	   // pause_symbols
		},

		// generic 16-bit encoder
		{
			// Encoder details
			"16-bits", // name
			true,	   // is_vuln_to_debruijn
			50,		   // repeat_min

			// Word and Symbol format (Address, Data, Sync)
			16,					// word_length
			"AAAAAAAAAAAAAAAA", // word_format

			// Symbol setup - Address + Data + Sync bit fragments
			"01",			  // symfield_address_symbols
			"01",			  // symfield_data_symbols
			4,				  // bit_fragments_length_per_symbol
			{"1000", "1110"}, // symbols_bit_fragments
			"",				  // sync_bit_fragment

			// Speed and clocks
			32,	   // period_per_symbol
			25000, // default_clk_speed
			0,	   // pause_symbols
		},

		// Test OOK Doorbell
		{
			// Encoder details
			"Doorbel", // name
			true,	   // is_vuln_to_debruijn
			32,		   // repeat_min

			// Word and Symbol format (Address, Data, Sync)
			24,							// word_length
			"AAAAAAAAAAAAAAAAAAAAAAAA", // word_format

			// Symbol setup - Address + Data + Sync bit fragments
			"01",			  // symfield_address_symbols
			"01",			  // symfield_data_symbols
			4,				  // bit_fragments_length_per_symbol
			{"1000", "1110"}, // symbols_bit_fragments
			"",				  // sync_bit_fragment

			// Speed and clocks
			228,	// period_per_symbol
			141260, // default_clk_speed
			32,		// pause_symbols
		},

		// Test OOK Garage Door
		{
			// Encoder details
			"OH200DC", // name
			true,	   // is_vuln_to_debruijn
			8,		   // repeat_min=230, looks like 8 is still working

			// Word and Symbol format (Address, Data, Sync)
			8,			// word_length
			"AAAAAAAA", // word_format

			// Symbol setup - Address + Data + Sync bit fragments
			"01",					  // symfield_address_symbols
			"01",					  // symfield_data_symbols
			8,						  // bit_fragments_length_per_symbol
			{"11110000", "10000000"}, // symbols_bit_fragments
			"",						  // sync_bit_fragment

			// Speed and clocks
			920,	// period_per_symbol
			285000, // default_clk_speed
			70,		// pause_symbols
		},

		// PT2260-R2
		{
			// Encoder details
			"2260-R2", // name
			false,	   // is_vuln_to_debruijn - false, as it contains a preamble and sync
			2,		   // repeat_min

			// Word and Symbol format (Address, Data, Sync)
			12,				 // word_length
			"AAAAAAAAAADDS", // word_format

			// Symbol setup - Address + Data + Sync bit fragments
			"01F",								  // symfield_address_symbols
			"01",								  // symfield_data_symbols
			8,									  // bit_fragments_length_per_symbol
			{"10001000", "11101110", "10001110"}, // symbols_bit_fragments
			"10000000000000000000000000000000",	  // sync_bit_fragment

			// Speed and clocks
			1024,	// period_per_symbol
			150000, // default_clk_speed
			0,		// pause_symbols
		},

		// PT2260-R4
		{
			// Encoder details
			"2260-R4", // name
			false,	   // is_vuln_to_debruijn - false, as it contains a preamble and sync
			2,		   // repeat_min

			// Word and Symbol format (Address, Data, Sync)
			12,				 // word_length
			"AAAAAAAADDDDS", // word_format

			// Symbol setup - Address + Data + Sync bit fragments
			"01F",								  // symfield_address_symbols
			"01",								  // symfield_data_symbols
			8,									  // bit_fragments_length_per_symbol
			{"10001000", "11101110", "10001110"}, // symbols_bit_fragments
			"10000000000000000000000000000000",	  // sync_bit_fragment

			// Speed and clocks
			1024,	// period_per_symbol
			150000, // default_clk_speed
			0,		// pause_symbols
		},

		// PT2262
		{
			// Encoder details
			"2262", // name
			false,	// is_vuln_to_debruijn - false, as it contains a preamble and sync
			4,		// repeat_min

			// Word and Symbol format (Address, Data, Sync)
			12,				 // word_length
			"AAAAAAAAAAAAS", // // word_format

			// Symbol setup - Address + Data + Sync bit fragments
			"01F",								  // symfield_address_symbols
			"01F",								  // symfield_data_symbols
			8,									  // bit_fragments_length_per_symbol
			{"10001000", "11101110", "10001110"}, // symbols_bit_fragments
			"10000000000000000000000000000000",	  // sync_bit_fragment

			// Speed and clocks
			32,	   // period_per_symbol
			20000, // default_clk_speed
			0,	   // pause_symbols
		},

		// 16-bit ?
		{
			// Encoder details
			"16-bit", // name
			false,	  // is_vuln_to_debruijn - false, as it contains a preamble and sync
			50,		  // repeat_min

			// Word and Symbol format (Address, Data, Sync)
			16,					 // word_length
			"AAAAAAAAAAAAAAAAS", // // word_format

			// Symbol setup - Address + Data + Sync bit fragments
			"01",					 // symfield_address_symbols
			"01",					 // symfield_data_symbols
			4,						 // bit_fragments_length_per_symbol
			{"1110", "1000"},		 // symbols_bit_fragments
			"100000000000000000000", // sync_bit_fragment

			// Speed and clocks
			32,	   // period_per_symbol
			25000, // default_clk_speed
			0,	   // pause_symbols
		},

		// RT1527
		{
			// Encoder details
			"1527", // name
			false,	// is_vuln_to_debruijn - false, as it contains a preamble and sync
			4,		// repeat_min

			// Word and Symbol format (Address, Data, Sync)
			24,							 // word_length
			"SAAAAAAAAAAAAAAAAAAAADDDD", // word_format

			// Symbol setup - Address + Data + Sync bit fragments
			"01",								// symfield_address_symbols
			"01",								// symfield_data_symbols
			4,									// bit_fragments_length_per_symbol
			{"1000", "1110"},					// symbols_bit_fragments
			"10000000000000000000000000000000", // sync_bit_fragment

			// Speed and clocks
			128,	// period_per_symbol
			100000, // default_clk_speed
			10,		// pause_symbols
		},

		// HK526E
		{
			// Encoder details
			"526E", // name
			true,	// is_vuln_to_debruijn - false, as it contains a preamble and sync
			4,		// repeat_min

			// Word and Symbol format (Address, Data, Sync)
			12,				// word_length
			"AAAAAAAAAAAA", // word_format

			// Symbol setup - Address + Data + Sync bit fragments
			"01",			// symfield_address_symbols
			"01",			// symfield_data_symbols
			3,				// bit_fragments_length_per_symbol
			{"110", "100"}, // symbols_bit_fragments
			"",				// sync_bit_fragment

			// Speed and clocks
			24,	   // period_per_symbol
			20000, // default_clk_speed
			10,	   // pause_symbols
		},

		// HT12E
		{
			// Encoder details
			"12E", // name
			false, // is_vuln_to_debruijn - false, as it contains a preamble and sync
			4,	   // repeat_min

			// Word and Symbol format (Address, Data, Sync)
			12,				 // word_length
			"SAAAAAAAADDDD", // word_format

			// Symbol setup - Address + Data + Sync bit fragments
			"01",									 // symfield_address_symbols
			"01",									 // symfield_data_symbols
			3,										 // bit_fragments_length_per_symbol
			{"011", "001"},							 // symbols_bit_fragments
			"0000000000000000000000000000000000001", // sync_bit_fragment

			// Speed and clocks
			3,	  // period_per_symbol
			3000, // default_clk_speed
			10,	  // pause_symbols
		},

		// VD5026 13 bits ?
		{
			// Encoder details
			"5026", // name
			false,	// is_vuln_to_debruijn - false, as it contains a preamble and sync
			4,		// repeat_min

			// Word and Symbol format (Address, Data, Sync)
			12,				 // word_length
			"SAAAAAAAAAAAA", // word_format

			// Symbol setup - Address + Data + Sync bit fragments
			"0123",																			  // symfield_address_symbols
			"0123",																			  // symfield_data_symbols
			16,																				  // bit_fragments_length_per_symbol
			{"1000000010000000", "1111111011111110", "1111111010000000", "1000000011111110"}, // symbols_bit_fragments
			"000000000000000000000000000000000000000000000001",								  // sync_bit_fragment						  // ?

			// Speed and clocks
			128,	// period_per_symbol
			100000, // default_clk_speed
			10,		// pause_symbols
		},

		// UM3750
		{
			// Encoder details
			"UM3750", // name
			false,	  // is_vuln_to_debruijn - false, as it contains a preamble and sync
			4,		  // repeat_min

			// Word and Symbol format (Address, Data, Sync)
			12,				 // word_length
			"SAAAAAAAAAAAA", // word_format

			// Symbol setup - Address + Data + Sync bit fragments
			"01",			// symfield_address_symbols
			"01",			// symfield_data_symbols
			3,				// bit_fragments_length_per_symbol
			{"011", "001"}, // symbols_bit_fragments

			// Encoder details
			"001", // sync_bit_fragment

			// Word and Symbol format (Address, Data, Sync)
			// Speed and clocks
			96,		// period_per_symbol
			100000, // default_clk_speed

			// Symbol setup - Address + Data + Sync bit fragments
			(3 * 12) - 6, // pause_symbols Compensates for pause delay bug in proc_ooktx
		},

		// Speed and clocks
		// UM3758
		{
			// Encoder details
			"UM3758", // name
			false,	  // is_vuln_to_debruijn - false, as it contains a preamble and sync
			4,		  // repeat_min

			// Word and Symbol format (Address, Data, Sync)
			18,					   // word_length
			"SAAAAAAAAAADDDDDDDD", // word_format

			// Symbol setup - Address + Data + Sync bit fragments
			"01F",							// symfield_address_symbols
			"01",							// symfield_data_symbols
			6,								// bit_fragments_length_per_symbol
			{"011011", "001001", "011001"}, // symbols_bit_fragments
			"1",							// sync_bit_fragment

			// Speed and clocks
			96,		// period_per_symbol
			160000, // default_clk_speed
			10,		// pause_symbols
		},

		// BA5104
		{
			// Encoder details
			"BA5104", // name
			false,	  // is_vuln_to_debruijn - false, as it contains a preamble and sync
			4,		  // repeat_min

			// Word and Symbol format (Address, Data, Sync)
			9,			  // word_length
			"SDDAAAAAAA", // word_format

			// Symbol setup - Address + Data + Sync bit fragments
			"01",			  // symfield_address_symbols
			"01",			  // symfield_data_symbols
			4,				  // bit_fragments_length_per_symbol
			{"1000", "1110"}, // symbols_bit_fragments
			"",				  // sync_bit_fragment

			// Speed and clocks
			3072,	// period_per_symbol
			455000, // default_clk_speed
			10,		// pause_symbols
		},

		// MC145026
		{
			// Encoder details
			"145026", // name
			false,	  // is_vuln_to_debruijn - false, as it contains a preamble and sync
			2,		  // repeat_min

			// Word and Symbol format (Address, Data, Sync)
			9,			  // word_length
			"SAAAAADDDD", // word_format

			// Symbol setup - Address + Data + Sync bit fragments
			"01F",														  // symfield_address_symbols
			"01",														  // symfield_data_symbols
			16,															  // bit_fragments_length_per_symbol
			{"0111111101111111", "0100000001000000", "0111111101000000"}, // symbols_bit_fragments
			"000000000000000000",										  // sync_bit_fragment

			// Speed and clocks
			16,		// period_per_symbol
			455000, // default_clk_speed
			2,		// pause_symbols
		},

		// HT6*** TODO: Add individual variations
		{
			// Encoder details
			"HT6***", // name
			false,	  // is_vuln_to_debruijn - false, as it contains a preamble and sync
			3,		  // repeat_min

			// Word and Symbol format (Address, Data, Sync)
			18,					   // word_length
			"SAAAAAAAAAAAADDDDDD", // word_format

			// Symbol setup - Address + Data + Sync bit fragments
			"01F",												 // symfield_address_symbols
			"01",												 // symfield_data_symbols
			6,													 // bit_fragments_length_per_symbol
			{"011011", "001001", "001011"},						 // symbols_bit_fragments
			"0000000000000000000000000000000000001011001011001", // sync_bit_fragment

			// Speed and clocks
			198,   // period_per_symbol
			80000, // default_clk_speed
			10,	   // pause_symbols
		},

		// TC9148
		{
			// Encoder details
			"TC9148", // name
			true,	  // is_vuln_to_debruijn - false, as it contains a preamble and sync
			3,		  // repeat_min

			// Word and Symbol format (Address, Data, Sync)
			12,				// word_length
			"AAAAAAAAAAAA", // word_format

			// Symbol setup - Address + Data + Sync bit fragments
			"01",			  // symfield_address_symbols
			"01",			  // symfield_data_symbols
			4,				  // bit_fragments_length_per_symbol
			{"1000", "1110"}, // symbols_bit_fragments
			"",				  // sync_bit_fragment

			// Speed and clocks
			48,		// period_per_symbol
			455000, // default_clk_speed
			10,		// pause_symbols
		},
	};

} /* namespace encoders */

#endif /*__ENCODERS_H__*/
