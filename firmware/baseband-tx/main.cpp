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

#include "baseband.hpp"
#include "baseband_dma.hpp"

#include "event_m4.hpp"

#include "irq_ipc_m4.hpp"

#include "touch_dma.hpp"

#include "modules.h"

#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "dsp_fft.hpp"
#include "dsp_fir_taps.hpp"
#include "dsp_iir.hpp"
#include "dsp_iir_config.hpp"
#include "dsp_squelch.hpp"

#include "channel_decimator.hpp"
#include "baseband_processor.hpp"
#include "proc_fsk_lcr.hpp"
#include "proc_jammer.hpp"
#include "proc_xylos.hpp"
#include "proc_playaudio.hpp"

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
#include <math.h>

static baseband::Direction direction = baseband::Direction::Receive;

class ThreadBase {
public:
	constexpr ThreadBase(
		const char* const name
	) : name { name }
	{
	}

	static msg_t fn(void* arg) {
		auto obj = static_cast<ThreadBase*>(arg);
		chRegSetThreadName(obj->name);
		obj->run();

		return 0;
	}

	virtual void run() = 0;

private:
	const char* const name;
};

class BasebandThread : public ThreadBase {
public:
	BasebandThread(
	) : ThreadBase { "baseband" }
	{
	}

	Thread* start(const tprio_t priority) {
		return chThdCreateStatic(wa, sizeof(wa),
			priority, ThreadBase::fn,
			this
		);
	}

	Thread* thread_main { nullptr };
	BasebandProcessor* baseband_processor { nullptr };
	BasebandConfiguration baseband_configuration;

private:
	WORKING_AREA(wa, 2048);

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

#define SAMPLES_PER_BIT 192
#define FILTER_SIZE 576
#define SAMPLE_BUFFER_SIZE SAMPLES_PER_BIT + FILTER_SIZE

static int32_t waveform_biphase[] = {
	165,167,168,168,167,166,163,160,
	157,152,147,141,134,126,118,109,
	99,88,77,66,53,41,27,14,
	0,-14,-29,-44,-59,-74,-89,-105,
	-120,-135,-150,-165,-179,-193,-206,-218,
	-231,-242,-252,-262,-271,-279,-286,-291,
	-296,-299,-301,-302,-302,-300,-297,-292,
	-286,-278,-269,-259,-247,-233,-219,-202,
	-185,-166,-145,-124,-101,-77,-52,-26,
	0,27,56,85,114,144,175,205,
	236,266,296,326,356,384,412,439,
	465,490,513,535,555,574,590,604,
	616,626,633,637,639,638,633,626,
	616,602,586,565,542,515,485,451,
	414,373,329,282,232,178,121,62,
	0,-65,-132,-202,-274,-347,-423,-500,
	-578,-656,-736,-815,-894,-973,-1051,-1128,
	-1203,-1276,-1347,-1415,-1479,-1540,-1596,-1648,
	-1695,-1736,-1771,-1799,-1820,-1833,-1838,-1835,
	-1822,-1800,-1767,-1724,-1670,-1605,-1527,-1437,
	-1334,-1217,-1087,-943,-785,-611,-423,-219,
	0,235,487,755,1040,1341,1659,1994,
	2346,2715,3101,3504,3923,4359,4811,5280,
	5764,6264,6780,7310,7856,8415,8987,9573,
	10172,10782,11404,12036,12678,13329,13989,14656,
	15330,16009,16694,17382,18074,18767,19461,20155,
	20848,21539,22226,22909,23586,24256,24918,25571,
	26214,26845,27464,28068,28658,29231,29787,30325,
	30842,31339,31814,32266,32694,33097,33473,33823,
	34144,34437,34699,34931,35131,35299,35434,35535,
	35602,35634,35630,35591,35515,35402,35252,35065,
	34841,34579,34279,33941,33566,33153,32702,32214,
	31689,31128,30530,29897,29228,28525,27788,27017,
	26214,25379,24513,23617,22693,21740,20761,19755,
	18725,17672,16597,15501,14385,13251,12101,10935,
	9755,8563,7360,6148,4927,3701,2470,1235,
	0,-1235,-2470,-3701,-4927,-6148,-7360,-8563,
	-9755,-10935,-12101,-13251,-14385,-15501,-16597,-17672,
	-18725,-19755,-20761,-21740,-22693,-23617,-24513,-25379,
	-26214,-27017,-27788,-28525,-29228,-29897,-30530,-31128,
	-31689,-32214,-32702,-33153,-33566,-33941,-34279,-34579,
	-34841,-35065,-35252,-35402,-35515,-35591,-35630,-35634,
	-35602,-35535,-35434,-35299,-35131,-34931,-34699,-34437,
	-34144,-33823,-33473,-33097,-32694,-32266,-31814,-31339,
	-30842,-30325,-29787,-29231,-28658,-28068,-27464,-26845,
	-26214,-25571,-24918,-24256,-23586,-22909,-22226,-21539,
	-20848,-20155,-19461,-18767,-18074,-17382,-16694,-16009,
	-15330,-14656,-13989,-13329,-12678,-12036,-11404,-10782,
	-10172,-9573,-8987,-8415,-7856,-7310,-6780,-6264,
	-5764,-5280,-4811,-4359,-3923,-3504,-3101,-2715,
	-2346,-1994,-1659,-1341,-1040,-755,-487,-235,
	0,219,423,611,785,943,1087,1217,
	1334,1437,1527,1605,1670,1724,1767,1800,
	1822,1835,1838,1833,1820,1799,1771,1736,
	1695,1648,1596,1540,1479,1415,1347,1276,
	1203,1128,1051,973,894,815,736,656,
	578,500,423,347,274,202,132,65,
	0,-62,-121,-178,-232,-282,-329,-373,
	-414,-451,-485,-515,-542,-565,-586,-602,
	-616,-626,-633,-638,-639,-637,-633,-626,
	-616,-604,-590,-574,-555,-535,-513,-490,
	-465,-439,-412,-384,-356,-326,-296,-266,
	-236,-205,-175,-144,-114,-85,-56,-27,
	0,26,52,77,101,124,145,166,
	185,202,219,233,247,259,269,278,
	286,292,297,300,302,302,301,299,
	296,291,286,279,271,262,252,242,
	231,218,206,193,179,165,150,135,
	120,105,89,74,59,44,29,14,
	0,-14,-27,-41,-53,-66,-77,-88,
	-99,-109,-118,-126,-134,-141,-147,-152,
	-157,-160,-163,-166,-167,-168,-168,-167
};

class RDSProcessor : public BasebandProcessor {
public:
	void execute(buffer_c8_t buffer) override {
        
		for (size_t i = 0; i<buffer.count; i++) {
			
			//Sample generation 2.28M/10=228kHz
			if(s >= 9) {
				s = 0;
				if(sample_count >= SAMPLES_PER_BIT) {
					cur_bit = (shared_memory.rdsdata[(bit_pos / 26) & 15]>>(25-(bit_pos % 26))) & 1;
					prev_output = cur_output;
					cur_output = prev_output ^ cur_bit;

					int32_t *src = waveform_biphase;
					int idx = in_sample_index;

					for(int j=0; j<FILTER_SIZE; j++) {
						val = (*src++);
						if (cur_output) val = -val;
						sample_buffer[idx++] += val;
						if (idx >= SAMPLE_BUFFER_SIZE) idx = 0;
					}

					in_sample_index += SAMPLES_PER_BIT;
					if (in_sample_index >= SAMPLE_BUFFER_SIZE) in_sample_index -= SAMPLE_BUFFER_SIZE;
					
					bit_pos++;
					sample_count = 0;
				}
				
				sample = sample_buffer[out_sample_index];
				sample_buffer[out_sample_index] = 0;
				out_sample_index++;
				if (out_sample_index >= SAMPLE_BUFFER_SIZE) out_sample_index = 0;
				
				//AM @ 228k/4=57kHz
				switch (mphase) {
					case 0:
					case 2: sample = 0; break;
					case 1: break;
					case 3: sample = -sample; break;
				}
				mphase++;
				if (mphase >= 4) mphase = 0;
				
				sample_count++;
			} else {
				s++;
			}
			
			//FM
			frq = (sample>>16) * 386760;
			
			phase = (phase + frq);
			sphase = phase + (256<<16);

			//re = sintab[(sphase & 0x03FF0000)>>16];
			//im = sintab[(phase & 0x03FF0000)>>16];
			
			buffer.p[i] = {(int8_t)re,(int8_t)im};
		}
	}

private:
	int8_t re, im;
	uint8_t mphase, s;
    uint32_t bit_pos;
    int32_t sample_buffer[SAMPLE_BUFFER_SIZE] = {0};
    int32_t val;
    uint8_t prev_output = 0;
    uint8_t cur_output = 0;
    uint8_t cur_bit = 0;
    int sample_count = SAMPLES_PER_BIT;
    int in_sample_index = 0;
    int32_t sample;
    int out_sample_index = SAMPLE_BUFFER_SIZE-1;
	uint32_t phase, sphase;
	int32_t sig, frq, frq_im, rdsc;
	int32_t k;
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

	touch::dma::init();

	const auto thread_main = chThdSelf();

	baseband_thread.thread_main = thread_main;

	baseband_thread.start(NORMALPRIO + 20);
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
		if( baseband_thread.baseband_processor ) {
			baseband_thread.baseband_processor->update_spectrum();
		}
	}
};

const auto baseband_buffer =
	new std::array<baseband::sample_t, 8192>();
	
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

	/* TODO: Ensure DMAs are configured to point at first LLI in chain. */

	touch::dma::allocate();
	touch::dma::enable();
	
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
