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

#include "encoders.hpp"
#include "portapack.hpp"

using namespace encoders;

#define CCIR_TONE_LENGTH (153600-1)		// 1536000*0.1
#define CCIR_DELTA_COEF (43.691)		// (65536*1024)/1536000
#define CCIR_SILENCE (614400-1)			// 400ms
	
struct bht_city {
	std::string name;
	uint8_t freq_index;
	bool recent;
};

const uint32_t ccir_deltas[16] = {
	(uint32_t)(1981 * CCIR_DELTA_COEF),
	(uint32_t)(1124 * CCIR_DELTA_COEF),
	(uint32_t)(1197 * CCIR_DELTA_COEF),
	(uint32_t)(1275 * CCIR_DELTA_COEF),
	(uint32_t)(1358 * CCIR_DELTA_COEF),
	(uint32_t)(1446 * CCIR_DELTA_COEF),
	(uint32_t)(1540 * CCIR_DELTA_COEF),
	(uint32_t)(1640 * CCIR_DELTA_COEF),
	(uint32_t)(1747 * CCIR_DELTA_COEF),
	(uint32_t)(1860 * CCIR_DELTA_COEF),
	(uint32_t)(2400 * CCIR_DELTA_COEF),
	(uint32_t)(930  * CCIR_DELTA_COEF),
	(uint32_t)(2247 * CCIR_DELTA_COEF),
	(uint32_t)(991  * CCIR_DELTA_COEF),
	(uint32_t)(2110 * CCIR_DELTA_COEF),
	(uint32_t)(1055 * CCIR_DELTA_COEF)
};

const rf::Frequency bht_freqs[7] = { 31325000, 31387500, 31437500, 31475000, 31687500, 31975000, 88000000 };

std::string gen_message_ep(uint8_t city_code, size_t family_code_ep, uint32_t relay_state_A, uint32_t relay_state_B);
std::string gen_message_xy(size_t header_code_a, size_t header_code_b, size_t city_code, size_t family_code,
							bool subfamily_wc, size_t subfamily_code, bool id_wc, size_t receiver_code,
							size_t relay_state_A, size_t relay_state_B, size_t relay_state_C, size_t relay_state_D);
std::string ccir_to_ascii(uint8_t * ccir);
