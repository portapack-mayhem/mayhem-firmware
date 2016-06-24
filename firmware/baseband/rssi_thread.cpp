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

#include "rssi_thread.hpp"

#include "rssi.hpp"
#include "rssi_dma.hpp"
#include "rssi_stats_collector.hpp"

#include "message.hpp"
#include "portapack_shared_memory.hpp"

WORKING_AREA(rssi_thread_wa, 128);

Thread* RSSIThread::thread = nullptr;

RSSIThread::RSSIThread(const tprio_t priority) {
	thread = chThdCreateStatic(rssi_thread_wa, sizeof(rssi_thread_wa),
		priority, ThreadBase::fn,
		this
	);
}

RSSIThread::~RSSIThread() {
	chThdTerminate(thread);
	chThdWait(thread);
	thread = nullptr;
}

void RSSIThread::run() {
	rf::rssi::init();
	rf::rssi::dma::allocate(4, 400);

	RSSIStatisticsCollector stats;

	rf::rssi::start();

	while( !chThdShouldTerminate() ) {
		// TODO: Place correct sampling rate into buffer returned here:
		const auto buffer_tmp = rf::rssi::dma::wait_for_buffer();
		const rf::rssi::buffer_t buffer {
			buffer_tmp.p, buffer_tmp.count, sampling_rate
		};

		stats.process(
			buffer,
			[](const RSSIStatistics& statistics) {
				const RSSIStatisticsMessage message { statistics };
				shared_memory.application_queue.push(message);
			}
		);
	}

	rf::rssi::stop();
	rf::rssi::dma::free();
}
