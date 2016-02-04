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

#include "lpc43xx_cpp.hpp"

#include "portapack_shared_memory.hpp"
#include "portapack_dma.hpp"

#include "gpdma.hpp"

#include "event_m4.hpp"

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
	
	chSysDisable();

	systick_stop();

	ShutdownMessage shutdown_message;
	shared_memory.application_queue.push(shutdown_message);

	halt();
}

int main(void) {
	init();

	/* TODO: Ensure DMAs are configured to point at first LLI in chain. */

	EventDispatcher event_dispatcher;
	event_dispatcher.run();

	shutdown();

	return 0;
}

/*
	void run() override {
		while(true) {
			if (direction == baseband::Direction::Transmit) {
				const auto buffer_tmp = baseband::dma::wait_for_tx_buffer();
				
				const buffer_c8_t buffer {
					buffer_tmp.p, buffer_tmp.count, baseband_configuration.sampling_rate
				};

				if( baseband_processor ) {
					baseband_processor->execute(buffer);
				}
			} else {
				const auto buffer_tmp = baseband::dma::wait_for_rx_buffer();
				
				const buffer_c8_t buffer {
					buffer_tmp.p, buffer_tmp.count, baseband_configuration.sampling_rate
				};

				if( baseband_processor ) {
					baseband_processor->execute(buffer);
				}
			}
		}
	}
};

class ToneProcessor : public BasebandProcessor {
public:
	void execute(buffer_c8_t buffer) override {
        
		for (size_t i = 0; i<buffer.count; i++) {
			
			//Sample generation 2.28M/10 = 228kHz
			if (s >= 9) {
				s = 0;
				aphase += 353205;	// DEBUG
				//sample = sintab[(aphase & 0x03FF0000)>>16];
			} else {
				s++;
			}
			
			//sample = sintab[(aphase & 0x03FF0000)>>16];
			
			//FM
			frq = sample * 500;		// DEBUG
			
			phase = (phase + frq);
			sphase = phase + (256<<16);

			//re = sintab[(sphase & 0x03FF0000)>>16];
			//im = sintab[(phase & 0x03FF0000)>>16];
			
			buffer.p[i] = {(int8_t)re,(int8_t)im};
		}
	}

private:
	int8_t re, im;
	uint8_t s;
    uint32_t sample_count;
	uint32_t aphase, phase, sphase;
	int32_t sample, sig, frq;
};

char ram_loop[32];
typedef int (*fn_ptr)(void);
fn_ptr loop_ptr;
	
void ram_loop_fn(void) {
	while(1) {}
}
	
void wait_for_switch(void) {
	memcpy(&ram_loop[0], reinterpret_cast<char*>(&ram_loop_fn), 32);
	loop_ptr = reinterpret_cast<fn_ptr>(&ram_loop[0]);
	ReadyForSwitchMessage message;
	shared_memory.application_queue.push(message);
	(*loop_ptr)();
}
		
int main(void) {
	init();

	events_initialize(chThdSelf());
	m0apptxevent_interrupt_enable();

	EventDispatcher event_dispatcher;
	auto& message_handlers = event_dispatcher.message_handlers();
	
	message_handlers.register_handler(Message::ID::ModuleID,
		[](Message* p) {
			ModuleIDMessage reply;
			auto message = static_cast<ModuleIDMessage*>(p);
			if (message->query == true) {	// Shouldn't be needed
				memcpy(reply.md5_signature, (const void *)(0x10087FF0), 16);
				reply.query = false;
				shared_memory.application_queue.push(reply);
			}
		}
	);

	message_handlers.register_handler(Message::ID::BasebandConfiguration,
		[&message_handlers](const Message* const p) {
			auto message = reinterpret_cast<const BasebandConfigurationMessage*>(p);
			if( message->configuration.mode != baseband_thread.baseband_configuration.mode ) {

				if( baseband_thread.baseband_processor ) {
					i2s::i2s0::tx_mute();
					baseband::dma::disable();
				}

				// TODO: Timing problem around disabling DMA and nulling and deleting old processor
				auto old_p = baseband_thread.baseband_processor;
				baseband_thread.baseband_processor = nullptr;
				delete old_p;

				switch(message->configuration.mode) {
				case TX_RDS:
					direction = baseband::Direction::Transmit;
					baseband_thread.baseband_processor = new RDSProcessor();
					break;
				
				case TX_LCR:
					direction = baseband::Direction::Transmit;
					baseband_thread.baseband_processor = new LCRFSKProcessor();
					break;
			
				case TX_TONE:
					direction = baseband::Direction::Transmit;
					baseband_thread.baseband_processor = new ToneProcessor();
					break;
				
				case TX_JAMMER:
					direction = baseband::Direction::Transmit;
					baseband_thread.baseband_processor = new JammerProcessor();
					break;
					
				case TX_XYLOS:
					direction = baseband::Direction::Transmit;
					baseband_thread.baseband_processor = new XylosProcessor();
					break;
					
				case PLAY_AUDIO:
					direction = baseband::Direction::Transmit;
					baseband_thread.baseband_processor = new PlayAudioProcessor();
					message_handlers.register_handler(Message::ID::FIFOData,
						[](Message* p) {
							auto message = static_cast<FIFODataMessage*>(p);
							baseband_thread.baseband_processor->fill_buffer(message->data);
						}
					);
					break;
					
				case SWITCH:
					wait_for_switch();

				default:
					break;
				}

				if( baseband_thread.baseband_processor )
					baseband::dma::enable(direction);
			}
			
			baseband::dma::configure(
				baseband_buffer->data(),
				direction
			);

			baseband_thread.baseband_configuration = message->configuration;
		}
	);

	message_handlers.register_handler(Message::ID::Shutdown,
		[&event_dispatcher](const Message* const) {
			event_dispatcher.request_stop();
		}
	);
*/
