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

#include "replay_thread.hpp"

#include "baseband_api.hpp"
#include "buffer_exchange.hpp"

struct BasebandReplay {
	BasebandReplay(ReplayConfig* const config) {
		baseband::replay_start(config);
	}

	~BasebandReplay() {
		baseband::replay_stop();
	}
};

// ReplayThread ///////////////////////////////////////////////////////////

ReplayThread::ReplayThread(
	std::unique_ptr<stream::Reader> reader,
	size_t read_size,
	size_t buffer_count,
	bool* ready_signal,
	std::function<void(uint32_t return_code)> terminate_callback
) : config { read_size, buffer_count },
	reader { std::move(reader) },
	ready_sig { ready_signal },
	terminate_callback { std::move(terminate_callback) }
{
	// Need significant stack for FATFS
	thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO + 10, ReplayThread::static_fn, this);
}

ReplayThread::~ReplayThread() {
	if( thread ) {
		chThdTerminate(thread);
		chThdWait(thread);
		thread = nullptr;
	}
}

msg_t ReplayThread::static_fn(void* arg) {
	auto obj = static_cast<ReplayThread*>(arg);
	const auto return_code = obj->run();
	if( obj->terminate_callback ) {
		obj->terminate_callback(return_code);
	}
	return 0;
}

uint32_t ReplayThread::run() {
	BasebandReplay replay { &config };
	BufferExchange buffers { &config };
	
	StreamBuffer* prefill_buffer { nullptr };
	
	// Wait for FIFOs to be allocated in baseband
	// Wait for ui_replay_view to tell us that the buffers are ready (awful :( )
	while (!(*ready_sig)) {
		chThdSleep(100);
	};
	
	// While empty buffers fifo is not empty...
	while (!buffers.empty()) {
		prefill_buffer = buffers.get_prefill();
		
		if (prefill_buffer == nullptr) {
			buffers.put_app(prefill_buffer);
		} else {
			size_t blocks = config.read_size / 512;
			
			for (size_t c = 0; c < blocks; c++) {
				auto read_result = reader->read(&((uint8_t*)prefill_buffer->data())[c * 512], 512);
				if( read_result.is_error() ) {
					return READ_ERROR;
				}
			}
			
			prefill_buffer->set_size(config.read_size);
			
			buffers.put(prefill_buffer);
		}
	};
	
	baseband::set_fifo_data(nullptr);

	while( !chThdShouldTerminate() ) {
		auto buffer = buffers.get();
		
		auto read_result = reader->read(buffer->data(), buffer->capacity());
		if( read_result.is_error() ) {
			return READ_ERROR;
		} else {
			if (read_result.value() == 0) {
				return END_OF_FILE;
			}
		}
		
		buffer->set_size(buffer->capacity());
		
		buffers.put(buffer);
	}

	return TERMINATED;
}
