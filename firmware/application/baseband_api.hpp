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

#ifndef __BASEBAND_API_H__
#define __BASEBAND_API_H__

#include "message.hpp"

#include "dsp_fir_taps.hpp"

#include "spi_image.hpp"

#include <cstddef>

namespace baseband {

struct AMConfig {
	const fir_taps_complex<64> channel;
	const AMConfigureMessage::Modulation modulation;

	void apply() const;
};

struct NBFMConfig {
	const fir_taps_real<24> decim_0;
	const fir_taps_real<32> decim_1;
	const fir_taps_real<32> channel;
	const size_t deviation;

	void apply() const;
};

struct WFMConfig {
	void apply() const;
};

void start(BasebandConfiguration configuration);
void stop();

void run_image(const portapack::spi_flash::region_t image_region);
void shutdown();

void spectrum_streaming_start();
void spectrum_streaming_stop();

void capture_start(CaptureConfig* const config);
void capture_stop();

} /* namespace baseband */

#endif/*__BASEBAND_API_H__*/
