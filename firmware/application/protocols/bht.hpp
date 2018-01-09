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
#include "ui_navigation.hpp"

#include "tonesets.hpp"
#include "encoders.hpp"

using namespace encoders;

#define XY_TONE_DURATION	((TONES_SAMPLERATE * 0.1) - 1)		// 100ms
#define XY_SILENCE 			(TONES_SAMPLERATE * 0.4)			// 400ms
#define XY_TONE_COUNT		20
#define XY_MAX_CITY			99

#define EPAR_BIT_DURATION	(OOK_SAMPLERATE / 580)
#define EPAR_REPEAT_COUNT	26
#define EPAR_MAX_CITY		255
	
struct bht_city {
	std::string name;
	uint8_t freq_index;
	bool recent;
};

size_t gen_message_ep(uint8_t city_code, size_t family_code_ep, uint32_t relay_state_A, uint32_t relay_state_B);
std::string gen_message_xy(const std::string& code);
std::string gen_message_xy(size_t header_code_a, size_t header_code_b, size_t city_code, size_t family_code,
							bool subfamily_wc, size_t subfamily_code, bool id_wc, size_t receiver_code,
							size_t relay_state_A, size_t relay_state_B, size_t relay_state_C, size_t relay_state_D);
std::string ccir_to_ascii(uint8_t * ccir);
