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

#ifndef __EVENT_M4_H__
#define __EVENT_M4_H__

#include "event.hpp"

#include "portapack_shared_memory.hpp"

#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "message.hpp"
#include "message_queue.hpp"

#include "ch.h"

#include "lpc43xx_cpp.hpp"

#include <array>

constexpr auto EVT_MASK_BASEBAND = EVENT_MASK(0);
constexpr auto EVT_MASK_SPECTRUM = EVENT_MASK(1);

class EventDispatcher {
public:
	void run() {
		events_initialize(chThdSelf());
		lpc43xx::creg::m0apptxevent::enable();

		baseband_thread.thread_main = chThdSelf();
		baseband_thread.thread_rssi = rssi_thread.start(NORMALPRIO + 10);
		baseband_thread.start(NORMALPRIO + 20);

		while(is_running) {
			const auto events = wait();
			dispatch(events);
		}

		lpc43xx::creg::m0apptxevent::disable();
	}

	void request_stop() {
		is_running = false;
	}

private:
	BasebandThread baseband_thread;
	RSSIThread rssi_thread;

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
		while(Message* const message = shared_memory.baseband_queue.peek(message_buffer)) {
			on_message(message);
			shared_memory.baseband_queue.skip();
		}
	}

	void on_message(const Message* const message) {
		switch(message->id) {
		case Message::ID::Shutdown:
			on_message_shutdown(*reinterpret_cast<const ShutdownMessage*>(message));
			break;

		default:
			on_message_default(message);
			break;
		}
	}

	void on_message_shutdown(const ShutdownMessage&) {
		request_stop();
	}

	void on_message_default(const Message* const message) {
		baseband_thread.on_message(message);
	}

	void handle_spectrum() {
		const UpdateSpectrumMessage message;
		baseband_thread.on_message(&message);
	}
};

#endif/*__EVENT_M4_H__*/
