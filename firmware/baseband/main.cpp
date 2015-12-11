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

#include "event_m4.hpp"

#include "irq_ipc_m4.hpp"

#include "touch_dma.hpp"

#include "baseband_thread.hpp"
#include "rssi_thread.hpp"
#include "baseband_processor.hpp"

#include "message_queue.hpp"

#include "utility.hpp"

#include "debug.hpp"

#include "audio.hpp"
#include "audio_dma.hpp"

#include "gcc.hpp"

#include <cstdint>
#include <cstddef>
#include <array>

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

static BasebandThread baseband_thread;
static RSSIThread rssi_thread;

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

	touch::dma::init();
	touch::dma::allocate();
	touch::dma::enable();
}

static void halt() {
	port_disable();
	while(true) {
		port_wait_for_interrupt();
	}
}

static void shutdown() {
	// TODO: Is this complete?
	
	nvicDisableVector(DMA_IRQn);

	m0apptxevent_interrupt_disable();
	
	chSysDisable();

	systick_stop();

	ShutdownMessage shutdown_message;
	shared_memory.application_queue.push(shutdown_message);

	halt();
}

class EventDispatcher {
public:
	MessageHandlerMap& message_handlers() {
		return message_map;
	}

	void run() {
		message_map.register_handler(Message::ID::BasebandConfiguration,
			[](const Message* const p) {
				auto message = reinterpret_cast<const BasebandConfigurationMessage*>(p);
				baseband_thread.set_configuration(message->configuration);
			}
		);

		message_map.register_handler(Message::ID::Shutdown,
			[this](const Message* const) {
				this->request_stop();
			}
		);

		events_initialize(chThdSelf());
		m0apptxevent_interrupt_enable();

		baseband_thread.thread_main = chThdSelf();
		baseband_thread.thread_rssi = rssi_thread.start(NORMALPRIO + 10);
		baseband_thread.start(NORMALPRIO + 20);

		while(is_running) {
			const auto events = wait();
			dispatch(events);
		}

		shutdown();
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
		if( baseband_thread.baseband_processor ) {
			baseband_thread.baseband_processor->update_spectrum();
		}
	}
};

int main(void) {
	init();

	/* TODO: Ensure DMAs are configured to point at first LLI in chain. */

	EventDispatcher event_dispatcher;
	event_dispatcher.run();

	return 0;
}
