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

#include "ch.h"
#include "test.h"

#include "lpc43xx_cpp.hpp"

#include "portapack_shared_memory.hpp"
#include "portapack_dma.hpp"

#include "gpdma.hpp"

#include "baseband_dma.hpp"

#include "event_m4.hpp"

#include "irq_ipc_m4.hpp"

#include "rssi.hpp"
#include "rssi_dma.hpp"

#include "touch_dma.hpp"

#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "dsp_fir_taps.hpp"
#include "dsp_iir.hpp"
#include "dsp_iir_config.hpp"
#include "dsp_squelch.hpp"

#include "baseband_stats_collector.hpp"
#include "rssi_stats_collector.hpp"

#include "channel_decimator.hpp"
#include "baseband_processor.hpp"
#include "proc_am_audio.hpp"
#include "proc_nfm_audio.hpp"
#include "proc_wfm_audio.hpp"
#include "proc_ais.hpp"
#include "proc_wideband_spectrum.hpp"
#include "proc_tpms.hpp"

#include "clock_recovery.hpp"
#include "packet_builder.hpp"

#include "message_queue.hpp"

#include "utility.hpp"

#include "debug.hpp"

#include "audio.hpp"
#include "audio_dma.hpp"

#include "gcc.hpp"

#include <cstdint>
#include <cstddef>
#include <array>
#include <string>
#include <bitset>

constexpr auto baseband_thread_priority = NORMALPRIO + 20;
constexpr auto rssi_thread_priority = NORMALPRIO + 10;

struct baseband_thread_data_t {
	Thread* thread_main { nullptr };
	Thread* thread_rssi { nullptr };
	BasebandProcessor* baseband_processor { nullptr };
	BasebandConfiguration baseband_configuration;
};

static WORKING_AREA(baseband_thread_wa, 8192);
static __attribute__((noreturn)) msg_t baseband_fn(void *arg) {
	auto thread_data = static_cast<const baseband_thread_data_t*>(arg);
	chRegSetThreadName("baseband");

	BasebandStatsCollector stats {
		chSysGetIdleThread(),
		thread_data->thread_main,
		thread_data->thread_rssi,
		chThdSelf()
	};

	while(true) {
		// TODO: Place correct sampling rate into buffer returned here:
		const auto buffer_tmp = baseband::dma::wait_for_rx_buffer();
		const buffer_c8_t buffer {
			buffer_tmp.p, buffer_tmp.count, thread_data->baseband_configuration.sampling_rate
		};

		if( thread_data->baseband_processor ) {
			thread_data->baseband_processor->execute(buffer);
		}

		stats.process(buffer,
			[](const BasebandStatistics statistics) {
				const BasebandStatisticsMessage message { statistics };
				shared_memory.application_queue.push(message);
			}
		);
	}
}

static WORKING_AREA(rssi_thread_wa, 128);
static __attribute__((noreturn)) msg_t rssi_fn(void *arg) {
	(void)arg;
	chRegSetThreadName("rssi");

	RSSIStatisticsCollector stats;

	while(true) {
		// TODO: Place correct sampling rate into buffer returned here:
		const auto buffer_tmp = rf::rssi::dma::wait_for_buffer();
		const rf::rssi::buffer_t buffer {
			buffer_tmp.p, buffer_tmp.count, 400000
		};

		stats.process(
			buffer,
			[](const RSSIStatistics statistics) {
				const RSSIStatisticsMessage message { statistics };
				shared_memory.application_queue.push(message);
			}
		);
	}
}

extern "C" {

void __late_init(void) {
	/*
	 * System initializations.
	 * - HAL initialization, this also initializes the configured device drivers
	 *   and performs the board-specific initializations.
	 * - Kernel initialization, the main() function becomes a thread and the
	 *   RTOS is active.
	 */
	halInit();

	/* After this call, scheduler, systick, heap, etc. are available. */
	/* By doing chSysInit() here, it runs before C++ constructors, which may
	 * require the heap.
	 */
	chSysInit();
}

}

static baseband_thread_data_t baseband_thread_data;

static void init() {
	i2s::i2s0::configure(
		audio::i2s0_config_tx,
		audio::i2s0_config_rx,
		audio::i2s0_config_dma
	);

	audio::dma::init();
	audio::dma::configure();
	audio::dma::enable();

	i2s::i2s0::tx_start();
	i2s::i2s0::rx_start();

	LPC_CREG->DMAMUX = portapack::gpdma_mux;
	gpdma::controller.enable();
	nvicEnableVector(DMA_IRQn, CORTEX_PRIORITY_MASK(LPC_DMA_IRQ_PRIORITY));

	baseband::dma::init();

	rf::rssi::init();
	touch::dma::init();

	const auto thread_main = chThdSelf();
	
	const auto thread_rssi = chThdCreateStatic(rssi_thread_wa, sizeof(rssi_thread_wa),
		rssi_thread_priority, rssi_fn,
		nullptr
	);

	baseband_thread_data.thread_main = thread_main;
	baseband_thread_data.thread_rssi = thread_rssi;

	chThdCreateStatic(baseband_thread_wa, sizeof(baseband_thread_wa),
		baseband_thread_priority, baseband_fn,
		&baseband_thread_data
	);
}

static void shutdown() {
	// TODO: Is this complete?
	
	nvicDisableVector(DMA_IRQn);

	m0apptxevent_interrupt_disable();
	
	chSysDisable();

	systick_stop();
}

static void halt() {
	port_disable();
	while(true) {
		port_wait_for_interrupt();
	}
}

class EventDispatcher {
public:
	MessageHandlerMap& message_handlers() {
		return message_map;
	}

	void run() {
		while(is_running) {
			const auto events = wait();
			dispatch(events);
		}
	}

	void request_stop() {
		is_running = false;
	}

private:
	MessageHandlerMap message_map;

	bool is_running = true;

	eventmask_t wait() {
		return chEvtWaitAny(ALL_EVENTS);
	}

	void dispatch(const eventmask_t events) {
		if( events & EVT_MASK_BASEBAND ) {
			handle_baseband_queue();
		}

		if( events & EVT_MASK_SPECTRUM ) {
			handle_spectrum();
		}
	}

	void handle_baseband_queue() {
		std::array<uint8_t, Message::MAX_SIZE> message_buffer;
		while(Message* const message = shared_memory.baseband_queue.pop(message_buffer)) {
			message_map.send(message);
		}
	}

	void handle_spectrum() {
		if( baseband_thread_data.baseband_processor ) {
			baseband_thread_data.baseband_processor->update_spectrum();
		}
	}
};

static constexpr auto direction = baseband::Direction::Receive;

int main(void) {
	init();

	events_initialize(chThdSelf());
	m0apptxevent_interrupt_enable();

	EventDispatcher event_dispatcher;
	auto& message_handlers = event_dispatcher.message_handlers();

	message_handlers.register_handler(Message::ID::BasebandConfiguration,
		[&message_handlers](const Message* const p) {
			auto message = reinterpret_cast<const BasebandConfigurationMessage*>(p);
			if( message->configuration.mode != baseband_thread_data.baseband_configuration.mode ) {

				if( baseband_thread_data.baseband_processor ) {
					i2s::i2s0::tx_mute();
					baseband::dma::disable();
					rf::rssi::stop();
				}

				// TODO: Timing problem around disabling DMA and nulling and deleting old processor
				auto old_p = baseband_thread_data.baseband_processor;
				baseband_thread_data.baseband_processor = nullptr;
				delete old_p;

				switch(message->configuration.mode) {
				case 0:
					baseband_thread_data.baseband_processor = new NarrowbandAMAudio();
					break;

				case 1:
					baseband_thread_data.baseband_processor = new NarrowbandFMAudio();
					break;

				case 2:
					baseband_thread_data.baseband_processor = new WidebandFMAudio();
					break;

				case 3:
					baseband_thread_data.baseband_processor = new AISProcessor();
					break;

				case 4:
					baseband_thread_data.baseband_processor = new WidebandSpectrum();
					break;

				case 5:
					baseband_thread_data.baseband_processor = new TPMSProcessor();
					break;

				default:
					break;
				}

				if( baseband_thread_data.baseband_processor ) {
					if( direction == baseband::Direction::Receive ) {
						rf::rssi::start();
					}
					baseband::dma::enable(direction);
				}
			}

			baseband_thread_data.baseband_configuration = message->configuration;
		}
	);

	message_handlers.register_handler(Message::ID::Shutdown,
		[&event_dispatcher](const Message* const) {
			event_dispatcher.request_stop();
		}
	);

	/* TODO: Ensure DMAs are configured to point at first LLI in chain. */

	if( direction == baseband::Direction::Receive ) {
		rf::rssi::dma::allocate(4, 400);
	}

	touch::dma::allocate();
	touch::dma::enable();

	const auto baseband_buffer =
		new std::array<baseband::sample_t, 8192>();
	baseband::dma::configure(
		baseband_buffer->data(),
		direction
	);
	//baseband::dma::allocate(4, 2048);

	event_dispatcher.run();

	shutdown();

	ShutdownMessage shutdown_message;
	shared_memory.application_queue.push(shutdown_message);

	halt();

	return 0;
}
