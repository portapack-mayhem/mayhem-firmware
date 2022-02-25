/*
 * Copyright (C) 2020 Belousov Oleg
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

#include "dsp_modulate.hpp"
#include "sine_table_int8.hpp"

namespace dsp {
namespace modulate {

Modulator::~Modulator() {
}

Mode Modulator::get_mode() {
	return mode;
}

void Modulator::set_mode(Mode new_mode) {
	mode = new_mode;
}

void Modulator::set_over(uint32_t new_over) {
    over = new_over;
}

///

SSB::SSB() : hilbert() {
	mode = Mode::LSB;
}

void SSB::execute(const buffer_s16_t& audio, const buffer_c8_t& buffer) {
	int32_t		sample = 0;
	int8_t		re = 0, im = 0;
	
	for (size_t counter = 0; counter < buffer.count; counter++) {
		if (counter % 128 == 0) {
			float	i = 0.0, q = 0.0;

		    sample = audio.p[counter / over] >> 2;
			//switch (mode) {
				//case Mode::LSB:	
			hilbert.execute(sample / 32768.0f, i, q);
				//case Mode::USB:	hilbert.execute(sample / 32768.0f, q, i);
				//default:	break;
			//}

			i *= 256.0f;   // Original 64.0f,  now x 4 (+12 dB's SSB BB modulation)	
			q *= 256.0f;   // Original 64.0f,  now x 4 (+12 dB's SSB BB modulation)	
			switch (mode) {
				case Mode::LSB:	re = q;	im = i;	break;
				case Mode::USB:	re = i;	im = q;	break;
				default:	re = 0; im = 0;	break;
			}
			//re = q;
			//im = i;
			//break;
		}
		
		buffer.p[counter] = { re, im };
	}
}

///

FM::FM() {
	mode = Mode::FM;
}

void FM::set_fm_delta(uint32_t new_delta) {
	fm_delta = new_delta;
}

void FM::execute(const buffer_s16_t& audio, const buffer_c8_t& buffer) {
	int32_t		sample = 0;
	int8_t		re, im;

	for (size_t counter = 0; counter < buffer.count; counter++) {
		if (counter % over == 0) {
		    sample = audio.p[counter / over] >> 8;
		    delta = sample * fm_delta;
		}

		phase += delta;
		sphase = phase >> 24;

		re = (sine_table_i8[(sphase + 64) & 255]);
		im = (sine_table_i8[sphase]);
		
		buffer.p[counter] = { re, im };
	}
}

AM::AM() {
	mode = Mode::AM;
}

void AM::execute(const buffer_s16_t& audio, const buffer_c8_t& buffer) {
	int32_t         sample = 0;
	int8_t          re = 0, im = 0;
	float		q = 0.0;
	
	for (size_t counter = 0; counter < buffer.count; counter++) {
		if (counter % 128 == 0) {
			sample = audio.p[counter / over] >> 2;
		}

		q = sample / 32768.0f;
		q *= 256.0f;										 // Original 64.0f,now x4 (+12 dB's BB_modulation in AM & DSB)	
		switch (mode) {
			case Mode::AM:	re = q + 80; im = q + 80; break; // Original DC add +20_DC_level=carrier,now x4 (+12dB's AM carrier)
			case Mode::DSB:	re = q; im = q; break;
			default:	break;
		}
		buffer.p[counter] = { re, im };
	}
}

}
}
