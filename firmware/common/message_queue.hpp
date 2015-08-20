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

class MessageQueue {
public:
	template<typename T>
	bool push(const T& message) {
		static_assert(sizeof(T) <= Message::MAX_SIZE, "Message::MAX_SIZE too small for message type");
		static_assert(std::is_base_of<Message, T>::value, "type is not based on Message");

		return push(&message, sizeof(message));
	}

	size_t pop(void* const buf, const size_t len);

	size_t len() const {
		return fifo.len();
	}

	bool is_empty() const {
		return fifo.is_empty();
	}

private:
	FIFO<uint8_t, 11> fifo;

	bool push(const void* const buf, const size_t len);

	void signal();
};

#endif/*__MESSAGE_QUEUE_H__*/
