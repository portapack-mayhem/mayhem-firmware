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

#ifndef __SSTV_H__
#define __SSTV_H__

namespace sstv {

#define SSTV_SAMPLERATE 3072000
#define SSTV_DELTA_COEF ((1ULL << 32) / SSTV_SAMPLERATE)

#define SSTV_F2D(f) (uint32_t)((f) * SSTV_DELTA_COEF)
#define SSTV_MS2S(d) (uint32_t)((d) / 1000.0 * (float)SSTV_SAMPLERATE)

#define SSTV_VIS_SS SSTV_F2D(1200)
#define SSTV_VIS_ZERO SSTV_F2D(1300)
#define SSTV_VIS_ONE SSTV_F2D(1100)

enum sstv_color_seq {
	SSTV_COLOR_RGB,
	SSTV_COLOR_GBR,
	SSTV_COLOR_YUV		// Not supported for now
};

#define SSTV_MODES_NB 6

// From http://www.graphics.stanford.edu/~seander/bithacks.html, nice !
constexpr inline uint8_t sstv_parity(uint8_t code) {
	uint8_t out = code;
	out ^= code >> 4;
	out &= 0x0F;
	return (((0b0110100110010110 >> out) & 1) << 7) | code;
}

struct sstv_tone {
	uint32_t frequency;
	uint32_t duration;
};

struct sstv_scanline {
	sstv_tone start_tone;
	sstv_tone gap_tone;
	uint8_t luma[320];
};

struct sstv_mode {
	char name[16];
	uint8_t vis_code;
	bool color;					// Unused for now
	sstv_color_seq color_sequence;
	uint16_t pixels;
	uint16_t lines;
	uint32_t samples_per_pixel;
	bool sync_on_first;
	uint8_t sync_index;
	bool gaps;
	uint32_t samples_per_sync;
	uint32_t samples_per_gap;
	//std::pair<uint16_t, uint16_t> luma_range;
};

constexpr sstv_mode sstv_modes[SSTV_MODES_NB] = {
	{ "Scottie 1", 	sstv_parity(60),	true, SSTV_COLOR_GBR, 320, 256, SSTV_MS2S(0.4320),	true, 2, true, SSTV_MS2S(9), SSTV_MS2S(1.5) },
	{ "Scottie 2", 	sstv_parity(56),	true, SSTV_COLOR_GBR, 320, 256, SSTV_MS2S(0.2752),	true, 2, true, SSTV_MS2S(9), SSTV_MS2S(1.5) },
	{ "Scottie DX",	sstv_parity(76),	true, SSTV_COLOR_GBR, 320, 256, SSTV_MS2S(1.08), 	true, 2, true, SSTV_MS2S(9), SSTV_MS2S(1.5) },
	{ "Martin 1",	sstv_parity(44),	true, SSTV_COLOR_GBR, 320, 256, SSTV_MS2S(0.4576),	false, 0, true, SSTV_MS2S(4.862), SSTV_MS2S(0.572) },
	{ "Martin 2",	sstv_parity(40),	true, SSTV_COLOR_GBR, 320, 256, SSTV_MS2S(0.2288),	false, 0, true, SSTV_MS2S(4.862), SSTV_MS2S(0.572) },
	{ "SC2-180",	sstv_parity(55),	true, SSTV_COLOR_RGB, 320, 256, SSTV_MS2S(0.7344), 	false, 0, false, SSTV_MS2S(5.5225), SSTV_MS2S(0.5) },
	//{ "PASOKON 3",	sstv_parity(113),	true, SSTV_COLOR_RGB, 640, 496, SSTV_MS2S(0.2083), 	{ 1500, 2300 } },
	//{ "PASOKON 7",	sstv_parity(115),	true, SSTV_COLOR_RGB, 640, 496, SSTV_MS2S(0.4167), 	{ 1500, 2300 } }
};

} /* namespace sstv */

#endif/*__SSTV_H__*/
