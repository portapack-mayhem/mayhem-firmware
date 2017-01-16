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

#ifndef __MESSAGE_QUEUE_H__
#define __MESSAGE_QUEUE_H__

#include <cstdint>

#include "message.hpp"
#include "fifo.hpp"

#include <ch.h>

class MessageQueue {
public:
	MessageQueue() = delete;
	MessageQueue(const MessageQueue&) = delete;
	MessageQueue(MessageQueue&&) = delete;
	
	MessageQueue(
		uint8_t* const data,
		size_t k
	) : fifo { data, k }
	{
		chMtxInit(&mutex_write);
	}

	template<typename T>
	bool push(const T& message) {
		static_assert(sizeof(T) <= Message::MAX_SIZE, "Message::MAX_SIZE too small for message type");
		static_assert(std::is_base_of<Message, T>::value, "type is not based on Message");

		return push(&message, sizeof(message));
	}

	template<typename T>
	bool push_and_wait(const T& message) {
		const bool result = push(message);
		if( result ) {
			// TODO: More graceful method of waiting for empty? Maybe sleep for a bit?
			while( !is_empty() );
		}
		return result;
	}

	template<typename HandlerFn>
	void handle(HandlerFn handler) {
		std::array<uint8_t, Message::MAX_SIZE> message_buffer;
		while(Message* const message = peek(message_buffer)) {
			handler(message);
			skip();
		}
	}

	bool is_empty() const {
		return fifo.is_empty();
	}

	void reset() {
		fifo.reset();
	}
	
private:
	FIFO<uint8_t> fifo;
	Mutex mutex_write { };

	Message* peek(std::array<uint8_t, Message::MAX_SIZE>& buf) {
		Message* const p = reinterpret_cast<Message*>(buf.data());
		return fifo.peek_r(buf.data(), buf.size()) ? p : nullptr;
	}

	bool skip() {
		return fifo.skip();
	}

	Message* pop(std::array<uint8_t, Message::MAX_SIZE>& buf) {
		Message* const p = reinterpret_cast<Message*>(buf.data());
		return fifo.out_r(buf.data(), buf.size()) ? p : nullptr;
	}

	size_t len() const {
		return fifo.len();
	}

	bool push(const void* const buf, const size_t len) {
		chMtxLock(&mutex_write);
		const auto result = fifo.in_r(buf, len);
		chMtxUnlock();

		const bool success = (result == len);
		if( success ) {
			signal();
		}
		return success;
	}

	void signal();
};

#endif/*__MESSAGE_QUEUE_H__*/
