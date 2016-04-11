/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __PROC_CAPTURE_HPP__
#define __PROC_CAPTURE_HPP__

#include "baseband_processor.hpp"
#include "dsp_decimate.hpp"

#include "stream_input.hpp"

#include <array>
#include <memory>

class CaptureProcessor : public BasebandProcessor {
public:
	CaptureProcessor();

	void execute(const buffer_c8_t& buffer) override;

private:
	std::array<complex16_t, 512> dst;
	const buffer_c16_t dst_buffer {
		dst.data(),
		dst.size()
	};

	dsp::decimate::FIRC8xR16x24FS4Decim4 decim_0;
	dsp::decimate::FIRC16xR16x16Decim2 decim_1;

	std::unique_ptr<StreamInput> stream;
};

#endif/*__PROC_CAPTURE_HPP__*/
