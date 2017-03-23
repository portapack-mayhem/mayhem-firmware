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
#define SSTV_MS2S(f) (uint32_t)((f) / 1000.0 * (float)SSTV_SAMPLERATE)

#define SSTV_VIS_SS SSTV_F2D(1200)
#define SSTV_VIS_ZERO SSTV_F2D(1300)
#define SSTV_VIS_ONE SSTV_F2D(1100)

struct sstv_tone {
	uint32_t frequency;
	uint32_t duration;
};

struct sstv_scanline {
	sstv_tone start_tone;
	sstv_tone gap_tone;
	uint8_t luma[320];
};

} /* namespace sstv */

#endif/*__SSTV_H__*/
