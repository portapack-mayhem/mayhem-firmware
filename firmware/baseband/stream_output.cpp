/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "stream_output.hpp"

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

StreamOutput::StreamOutput(ReplayConfig* const config) :
	fifo_buffers_empty { buffers_empty.data(), buffer_count_max_log2 },
	fifo_buffers_full { buffers_full.data(), buffer_count_max_log2 },
	config { config },
	data { std::make_unique<uint8_t[]>(config->read_size * config->buffer_count) }
{
	config->fifo_buffers_empty = &fifo_buffers_empty;
	config->fifo_buffers_full = &fifo_buffers_full;

	for(size_t i=0; i<config->buffer_count; i++) {
		buffers[i] = { &(data.get()[i * config->read_size]), config->read_size };
		fifo_buffers_empty.in(&buffers[i]);
	}
}

size_t StreamOutput::read(void* const data, const size_t length) {
	uint8_t* p = static_cast<uint8_t*>(data);
	size_t read = 0;

	while( read < length ) {
		if( !active_buffer ) {
			// We need a full buffer...
			if( !fifo_buffers_full.out(active_buffer) ) {
				// ...but none are available. Samples were dropped.
				break;
			}
		}
		
		const auto remaining = length - read;
		read += active_buffer->read(&p[read], remaining);
		//buffer->empty();

		if( active_buffer->is_empty() ) {
			if( !fifo_buffers_empty.in(active_buffer) ) {
				// FIFO is completly empty.
				// Bail out of the loop, and try retrieving the buffer in the
				// next pass.
				break;
			}
			active_buffer = nullptr;
			creg::m4txevent::assert();
		}
	}

	config->baseband_bytes_received += length;

	return read;
}
