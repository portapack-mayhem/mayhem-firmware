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
	StreamInput(const size_t K) :
		K { K },
		data { std::make_unique<uint8_t[]>(1UL << K) },
		fifo { data.get(), K }
	{
		// TODO: Send stream creation message.
		shared_memory.FIFO_HACK = &fifo;
	}

	~StreamInput() {
		// TODO: Send stream distruction message.
		shared_memory.FIFO_HACK = nullptr;
	}

	size_t write(const void* const data, const size_t length) {
		const auto written = fifo.in(reinterpret_cast<const uint8_t*>(data), length);

		const auto last_bytes_written = bytes_written;
		bytes_written += written;
		if( (bytes_written & event_bytes_mask) < (last_bytes_written & event_bytes_mask) ) {
			creg::m4txevent::assert();
		}

		return written;
	}

	uint64_t written() const {
		return bytes_written;
	}

private:
	const size_t K;
	const uint64_t event_bytes_mask = (1ULL << (K - 2)) - 1;
	uint64_t bytes_written = 0;
	std::unique_ptr<uint8_t[]> data;
	FIFO<uint8_t> fifo;
};

#endif/*__STREAM_INPUT_H__*/
