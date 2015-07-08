/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "message_queue.hpp"

#include "ch.h"
#include "lpc43xx_cpp.hpp"

using namespace lpc43xx;

bool MessageQueue::push(Message* const message) {
	/* Returns true if success:
	 * - Message not in use.
	 * - FIFO wasn't full.
	 */
	if( message->state == Message::State::Free ) {
		message->state = Message::State::InUse;

		if(	enqueue(message) ) {
			signal();
			return true;
		} else {
			// Roll back message state.
			message->state = Message::State::Free;
		}
	}

	return false;
}

Message* MessageQueue::pop() {
	/* TODO: Because of architecture characteristics, the two LSBs of the
	 * message pointer will always be 0. Other (non-pointer) message types
	 * could be encoded by setting these two bits to non-zero values.
	 * One of the bits could also be used as an "ack" flag... In fact, a
	 * pointer message could be turned into an "ack" message, or something
	 * like that...
	 * Might be better though to use formal operating structures in the
	 * message to do synchronization between processors.
	 */
	Message* message { nullptr };
	const auto success = fifo.out(&message, 1);
	return success ? message : nullptr;
}

#if defined(LPC43XX_M0)
bool MessageQueue::enqueue(Message* const message) {
	return fifo.in(&message, 1);
}

void MessageQueue::signal() {
	creg::m0apptxevent::assert();
}
#endif

#if defined(LPC43XX_M4)
bool MessageQueue::enqueue(Message* const message) {
	return fifo.in(&message, 1);
}

void MessageQueue::signal() {
	creg::m4txevent::assert();
}
#endif
