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
	bool push(Message* const message);

	Message* pop();

	size_t len() const {
		return fifo.len();
	}

	bool is_empty() const {
		return fifo.is_empty();
	}

private:
	FIFO<Message*, 8> fifo;

	bool enqueue(Message* const message);
	void signal();
};

#endif/*__MESSAGE_QUEUE_H__*/
