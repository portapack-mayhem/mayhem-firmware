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
#include "baseband_stats_collector.hpp"
#include "baseband_dma.hpp"

#include "rssi.hpp"
#include "i2s.hpp"

#include "proc_am_audio.hpp"
#include "proc_nfm_audio.hpp"
#include "proc_wfm_audio.hpp"
#include "proc_ais.hpp"
#include "proc_wideband_spectrum.hpp"
#include "proc_tpms.hpp"
#include "proc_ert.hpp"

#include "portapack_shared_memory.hpp"

#include <array>

Thread* BasebandThread::start(const tprio_t priority) {
	return chThdCreateStatic(wa, sizeof(wa),
		priority, ThreadBase::fn,
		this
	);
}

void BasebandThread::set_configuration(const BasebandConfiguration& new_configuration) {
	if( new_configuration.mode != baseband_configuration.mode ) {
		disable();

		// TODO: Timing problem around disabling DMA and nulling and deleting old processor
		auto old_p = baseband_processor;
		baseband_processor = nullptr;
		delete old_p;

		baseband_processor = create_processor(new_configuration.mode);

		enable();
	}

	baseband_configuration = new_configuration;
}

void BasebandThread::run() {
	baseband::dma::init();

	const auto baseband_buffer = new std::array<baseband::sample_t, 8192>();
	baseband::dma::configure(
		baseband_buffer->data(),
		direction()
	);
	//baseband::dma::allocate(4, 2048);

	BasebandStatsCollector stats {
		chSysGetIdleThread(),
		thread_main,
		thread_rssi,
		chThdSelf()
	};

	while(true) {
		// TODO: Place correct sampling rate into buffer returned here:
		const auto buffer_tmp = baseband::dma::wait_for_rx_buffer();
		buffer_c8_t buffer {
			buffer_tmp.p, buffer_tmp.count, baseband_configuration.sampling_rate
		};

		if( baseband_processor ) {
			baseband_processor->execute(buffer);
		}

		stats.process(buffer,
			[](const BasebandStatistics& statistics) {
				const BasebandStatisticsMessage message { statistics };
				shared_memory.application_queue.push(message);
			}
		);
	}

	delete baseband_buffer;
}

BasebandProcessor* BasebandThread::create_processor(const int32_t mode) {
	switch(mode) {
	case 0:		return new NarrowbandAMAudio();
	case 1:		return new NarrowbandFMAudio();
	case 2:		return new WidebandFMAudio();
	case 3:		return new AISProcessor();
	case 4:		return new WidebandSpectrum();
	case 5:		return new TPMSProcessor();
	case 6:		return new ERTProcessor();
	default:	return nullptr;
	}
}

void BasebandThread::disable() {
	if( baseband_processor ) {
		i2s::i2s0::tx_mute();
		baseband::dma::disable();
		rf::rssi::stop();
	}
}

void BasebandThread::enable() {
	if( baseband_processor ) {
		if( direction() == baseband::Direction::Receive ) {
			rf::rssi::start();
		}
		baseband::dma::enable(direction());
	}
}
