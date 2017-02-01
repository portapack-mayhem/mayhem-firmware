/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "baseband_thread.hpp"

#include "dsp_types.hpp"

#include "baseband.hpp"
#include "baseband_sgpio.hpp"
#include "baseband_dma.hpp"

#include "rssi.hpp"
#include "i2s.hpp"
using namespace lpc43xx;

#include "portapack_shared_memory.hpp"

#include "utility.hpp"

#include <array>

static baseband::SGPIO baseband_sgpio;

WORKING_AREA(baseband_thread_wa, 4096);

Thread* BasebandThread::thread = nullptr;

BasebandThread::BasebandThread(
	uint32_t sampling_rate,
	BasebandProcessor* const baseband_processor,
	const tprio_t priority,
	baseband::Direction direction
) : baseband_processor { baseband_processor },
	_direction { direction },
	sampling_rate { sampling_rate }
{
	thread = chThdCreateStatic(baseband_thread_wa, sizeof(baseband_thread_wa),
		priority, ThreadBase::fn,
		this
	);
}

BasebandThread::~BasebandThread() {
	chThdTerminate(thread);
	chThdWait(thread);
	thread = nullptr;
}

void BasebandThread::set_sampling_rate(uint32_t new_sampling_rate) {
	sampling_rate = new_sampling_rate;
}

void BasebandThread::run() {
	baseband_sgpio.init();
	baseband::dma::init();

	const auto baseband_buffer = std::make_unique<std::array<baseband::sample_t, 8192>>();
	baseband::dma::configure(
		baseband_buffer->data(),
		direction()
	);
	//baseband::dma::allocate(4, 2048);

	baseband_sgpio.configure(direction());
	baseband::dma::enable(direction());
	baseband_sgpio.streaming_enable();

	while( !chThdShouldTerminate() ) {
		// TODO: Place correct sampling rate into buffer returned here:
		const auto buffer_tmp = baseband::dma::wait_for_buffer();
		if( buffer_tmp ) {
			buffer_c8_t buffer {
				buffer_tmp.p, buffer_tmp.count, sampling_rate
			};

			if( baseband_processor ) {
				baseband_processor->execute(buffer);
			}
		}
	}

	i2s::i2s0::tx_mute();
	baseband::dma::disable();
	baseband_sgpio.streaming_disable();
}
