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

#ifndef __DSP_MODULATE_H__
#define __DSP_MODULATE_H__

#include "dsp_types.hpp"
#include "dsp_hilbert.hpp"

namespace dsp {
namespace modulate {

enum class Mode {
	None,
	AM,
	DSB,
	LSB,
	USB,
	FM
};

///

class Modulator {
public:
	virtual void execute(const buffer_s16_t& audio, const buffer_c8_t& buffer) = 0;
	virtual ~Modulator();

	Mode get_mode();
	void set_mode(Mode new_mode);

    void set_over(uint32_t new_over);

protected:
    uint32_t    over = 1;
	Mode	    mode = Mode::None;
};

///

class SSB : public Modulator {
public:
	SSB();

	virtual void execute(const buffer_s16_t& audio, const buffer_c8_t& buffer);

private:
	dsp::HilbertTransform	hilbert;
};

///

class FM : public Modulator {
public:
	FM();

	virtual void execute(const buffer_s16_t& audio, const buffer_c8_t& buffer);
	void set_fm_delta(uint32_t new_delta);

///

private:
	uint32_t	fm_delta { 0 };
	uint32_t	phase { 0 }, sphase { 0 };
	int32_t		sample { 0 }, delta { };
};

class AM : public Modulator {
public:
        AM();

        virtual void execute(const buffer_s16_t& audio, const buffer_c8_t& buffer);
};

} /* namespace modulate */
} /* namespace dsp */

#endif/*__DSP_MODULATE_H__*/
