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

#ifndef __AUDIO_INPUT_H__
#define __AUDIO_INPUT_H__

#include "dsp_types.hpp"

#include "dsp_iir.hpp"
#include "dsp_squelch.hpp"

#include "stream_input.hpp"

#include <cstdint>
#include <memory>

class AudioInput {
public:
	void read_audio_buffer(buffer_s16_t& audio);

private:
	/*static constexpr float k = 32768.0f;
	static constexpr float ki = 1.0f / k;

	IIRBiquadFilter hpf { };*/
};

#endif/*__AUDIO_INPUT_H__*/
