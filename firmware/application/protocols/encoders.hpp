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

#define ENC_TYPES_COUNT 20
#define ENCODER_UM3750 14
#define OOK_SAMPLERATE 2280000U
#define OOK_DEFAULT_STEP 8 // 70 kHz carrier frequency

	size_t make_bitstream(std::string &fragments);
	void bitstream_append(size_t &bitstream_length, uint32_t bit_count, uint32_t bits);

	struct encoder_def_t
	{
		char name[16];			   // Encoder chip ref/name
		char address_symbols[8];   // List of possible symbols like "01", "01F"...
		char data_symbols[8];	   // Same
		uint16_t clk_per_symbol;   // Oscillator periods per symbol
		uint16_t clk_per_fragment; // Oscillator periods per symbol fragment (state)
		char bit_format[4][20];	   // List of fragments for each symbol in previous *_symbols list order
		uint8_t word_length;	   // Total # of symbols (not counting sync)
		char word_format[32];	   // A for Address, D for Data, S for sync
		char sync[64];			   // Like bit_format
		uint32_t default_speed;	   // Default encoder clk frequency (often set by shitty resistor)
		uint8_t repeat_min;		   // Minimum repeat count
		uint16_t pause_symbols;	   // Length of pause between repeats in symbols
		bool is_vuln_to_debruijn;  // True if the encoder is vulnerable to the debruijn attack
	};

	// Warning ! If this is changed, make sure that ENCODER_UM3750 is still valid !
	constexpr encoder_def_t encoder_defs[ENC_TYPES_COUNT] = {
		// generic 8-bit encoder
		{
			"g8bit",
			"01",
			"01",
			32,
			8,
			{"1000", "1110"},
			8,
			"AAAAAAAA",
			"",
			25000,
			50,
			0,
			true,
		},

		// generic 16-bit encoder
		{
			"g16bit",
			"01",
			"01",
			32,
			8,
			{"1000", "1110"},
			16,
			"AAAAAAAAAAAAAAAA",
			"",
			25000,
			50,
			0,
			true,
		},

		// generic 8-bit encoder reversed
		{
			"g8bitr",
			"01",
			"01",
			32,
			8,
			{"1110",
			 "1000"},
			8,
			"AAAAAAAA",
			"",
			25000,
			50,
			0,
			true,
		},

		// generic 16-bit encoder reversed
		{
			"g16bitr",
			"01",
			"01",
			32,
			8,
			{"1110",
			 "1000"},
			16,
			"AAAAAAAAAAAAAAAA",
			"",
			25000,
			50,
			0,
			true,
		},

		// Test OOK Doorbell
		{
			"Doorbel",
			"01",
			"01",
			228,
			57,
			{"1000", "1110"},
			24,
			"AAAAAAAAAAAAAAAAAAAAAAAA",
			"",
			141260,
			32, // repeat=32
			32,
			true,
		},

		// Test OOK Garage Door
		{
			"OH200DC",
			"01",
			"01",
			920,
			115,
			{"10000000", "11110000"},
			8,
			"AAAAAAAA",
			"",
			285000,
			8, // repeat=230, looks like 8 is still working
			70,
			true,
		},

		// PT2260-R2
		{
			"2260-R2",
			"01F",
			"01",
			1024,
			128,
			{"10001000", "11101110", "10001110"},
			12,
			"AAAAAAAAAADDS",
			"10000000000000000000000000000000",
			150000,
			2,
			0,
			false, // as it contains a preamble and sync
		},

		// PT2260-R4
		{
			"2260-R4",
			"01F",
			"01",
			1024,
			128,
			{"10001000", "11101110", "10001110"},
			12,
			"AAAAAAAADDDDS",
			"10000000000000000000000000000000",
			150000,
			2,
			0,
			false, // as it contains a preamble and sync
		},

		// PT2262
		{
			"2262",
			"01F",
			"01F",
			32,
			4,
			{"10001000", "11101110", "10001110"},
			12,
			"AAAAAAAAAAAAS",
			"10000000000000000000000000000000",
			20000,
			4,
			0,
			false, // as it contains a preamble and sync
		},

		// 16-bit ?
		{
			"16-bit",
			"01",
			"01",
			32,
			8,
			{"1110", "1000"}, // Opposite ?
			16,
			"AAAAAAAAAAAAAAAAS",
			"100000000000000000000",
			25000,
			50,
			0,	   // ?
			false, // as it contains a preamble and sync
		},

		// RT1527
		{
			"1527",
			"01",
			"01",
			128,
			32,
			{"1000", "1110"},
			24,
			"SAAAAAAAAAAAAAAAAAAAADDDD",
			"10000000000000000000000000000000",
			100000,
			4,
			10,	   // ?
			false, // as it contains a preamble and sync
		},

		// HK526E
		{
			"526E",
			"01",
			"01",
			24,
			8,
			{"110", "100"},
			12,
			"AAAAAAAAAAAA",
			"",
			20000,
			4,
			10, // ?
			true,
		},

		// HT12E
		{
			"12E",
			"01",
			"01",
			3,
			1,
			{"011", "001"},
			12,
			"SAAAAAAAADDDD",
			"0000000000000000000000000000000000001",
			3000,
			4,
			10,	   // ?
			false, // as it contains a preamble and sync
		},

		// VD5026 13 bits ?
		{
			"5026",
			"0123",
			"0123",
			128,
			8,
			{"1000000010000000", "1111111011111110", "1111111010000000", "1000000011111110"},
			12,
			"SAAAAAAAAAAAA",
			"000000000000000000000000000000000000000000000001", // ?
			100000,
			4,
			10,	   // ?
			false, // as it contains a preamble and sync
		},

		// UM3750
		{
			"UM3750",
			"01",
			"01",
			96,
			32,
			{"011", "001"},
			12,
			"SAAAAAAAAAAAA",
			"001",
			100000,
			4,
			(3 * 12) - 6, // Compensates for pause delay bug in proc_ook
			false,		  // as it contains a preamble and sync
		},

		// UM3758
		{
			"UM3758",
			"01F",
			"01",
			96,
			16,
			{"011011", "001001", "011001"},
			18,
			"SAAAAAAAAAADDDDDDDD",
			"1",
			160000,
			4,
			10,	   // ?
			false, // as it contains a preamble and sync
		},

		// BA5104
		{
			"BA5104",
			"01",
			"01",
			3072,
			768,
			{"1000", "1110"},
			9,
			"SDDAAAAAAA",
			"",
			455000,
			4,
			10,	   // ?
			false, // as it contains a preamble and sync
		},

		// MC145026
		{
			"145026",
			"01F",
			"01",
			16,
			1,
			{"0111111101111111", "0100000001000000", "0111111101000000"},
			9,
			"SAAAAADDDD",
			"000000000000000000",
			455000,
			2,
			2,
			false, // as it contains a preamble and sync
		},

		// HT6*** TODO: Add individual variations
		{
			"HT6***",
			"01F",
			"01",
			198,
			33,
			{"011011", "001001", "001011"},
			18,
			"SAAAAAAAAAAAADDDDDD",
			"0000000000000000000000000000000000001011001011001",
			80000,
			3,
			10,	   // ?
			false, // as it contains a preamble and sync
		},

		// TC9148
		{
			"TC9148",
			"01",
			"01",
			48,
			12,
			{
				"1000",
				"1110",
			},
			12,
			"AAAAAAAAAAAA",
			"",
			455000,
			3,
			10, // ?
			true,
		},
	};

} /* namespace encoders */

#endif /*__ENCODERS_H__*/
