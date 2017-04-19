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

#include "stream_input.hpp"

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

StreamInput::StreamInput(CaptureConfig* const config) :
	fifo_buffers_empty { buffers_empty.data(), buffer_count_max_log2 },
	fifo_buffers_full { buffers_full.data(), buffer_count_max_log2 },
	config { config },
	data { std::make_unique<uint8_t[]>(config->write_size * config->buffer_count) }
{
	config->fifo_buffers_empty = &fifo_buffers_empty;
	config->fifo_buffers_full = &fifo_buffers_full;

	for(size_t i=0; i<config->buffer_count; i++) {
		buffers[i] = { &(data.get()[i * config->write_size]), config->write_size };
		fifo_buffers_empty.in(&buffers[i]);
	}
}

size_t StreamInput::write(const void* const data, const size_t length) {
	const uint8_t* p = static_cast<const uint8_t*>(data);
	size_t written = 0;

	while( written < length ) {
		if( !active_buffer ) {
			// We need an empty buffer...
			if( !fifo_buffers_empty.out(active_buffer) ) {
				// ...but none are available. Samples were dropped.
				break;
			}
		}
		
		const auto remaining = length - written;
		written += active_buffer->write(&p[written], remaining);

		if( active_buffer->is_full() ) {
			if( !fifo_buffers_full.in(active_buffer) ) {
				// FIFO is full of buffers, there's no place for this one.
				// Bail out of the loop, and try submitting the buffer in the
				// next pass.
				// This should never happen if the number of buffers is less
				// than the capacity of the FIFO.
				break;
			}
			active_buffer = nullptr;
			creg::m4txevent::assert();
		}
	}

	config->baseband_bytes_received += length;
	config->baseband_bytes_dropped += (length - written);

	return written;
}
