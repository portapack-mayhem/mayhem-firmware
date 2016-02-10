/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __STREAM_INPUT_H__
#define __STREAM_INPUT_H__

#include "portapack_shared_memory.hpp"

#include "fifo.hpp"

#include <cstdint>
#include <cstddef>
#include <memory>

class StreamInput {
public:
	StreamInput() : fifo { std::make_unique<FIFO<uint8_t, 13>>() }
	{
		// TODO: Send stream creation message.
		shared_memory.FIFO_HACK = fifo.get();
	}

	~StreamInput() {
		// TODO: Send stream distruction message.
		shared_memory.FIFO_HACK = nullptr;
	}

	size_t write(const void* const data, const size_t length) {
		return fifo->in(reinterpret_cast<const uint8_t*>(data), length);
	}

private:
	std::unique_ptr<FIFO<uint8_t, 13>> fifo;
};

#endif/*__STREAM_INPUT_H__*/
