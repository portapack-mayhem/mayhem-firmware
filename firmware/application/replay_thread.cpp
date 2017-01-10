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

// StreamOutput ///////////////////////////////////////////////////////////

class StreamOutput {
public:
	StreamOutput(CaptureConfig* const config);
	~StreamOutput();

	size_t available() {
		return fifo_buffers_full->len();
	}

	StreamBuffer* get_buffer() {
		StreamBuffer* p { nullptr };
		fifo_buffers_full->out(p);
		return p;
	}

	bool release_buffer(StreamBuffer* const p) {
		p->empty();
		return fifo_buffers_empty->in(p);
	}

	static FIFO<StreamBuffer*>* fifo_buffers_empty;
	static FIFO<StreamBuffer*>* fifo_buffers_full;

private:
	CaptureConfig* const config;
};

FIFO<StreamBuffer*>* StreamOutput::fifo_buffers_empty = nullptr;
FIFO<StreamBuffer*>* StreamOutput::fifo_buffers_full = nullptr;

StreamOutput::StreamOutput(
	CaptureConfig* const config
) : config { config }
{
	baseband::capture_start(config);
	fifo_buffers_empty = config->fifo_buffers_empty;
	fifo_buffers_full = config->fifo_buffers_full;
}

StreamOutput::~StreamOutput() {
	fifo_buffers_full = nullptr;
	fifo_buffers_empty = nullptr;
	baseband::capture_stop();
}

// CaptureThread //////////////////////////////////////////////////////////

Thread* ReplayThread::thread = nullptr;

ReplayThread::ReplayThread(
	std::unique_ptr<Reader> reader,
	size_t read_size,
	size_t buffer_count,
	std::function<void()> success_callback,
	std::function<void(File::Error)> error_callback
) : config { read_size, buffer_count },
	reader { std::move(reader) },
	success_callback { std::move(success_callback) },
	error_callback { std::move(error_callback) }
{
	// Need significant stack for FATFS
	thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO + 10, ReplayThread::static_fn, this);
}

ReplayThread::~ReplayThread() {
	if( thread ) {
		chThdTerminate(thread);
		chEvtSignal(thread, event_mask_loop_wake);
		chThdWait(thread);
		thread = nullptr;
	}
}

void ReplayThread::check_fifo_isr() {
	// TODO: Prevent over-signalling by transmitting a set of 
	// flags from the baseband core.
	const auto fifo = StreamOutput::fifo_buffers_full;
	if( fifo ) {
		if( !fifo->is_empty() ) {
			chEvtSignalI(thread, event_mask_loop_wake);
		}
	}
}

msg_t ReplayThread::static_fn(void* arg) {
	auto obj = static_cast<ReplayThread*>(arg);
	const auto error = obj->run();
	if( error.is_valid() && obj->error_callback ) {
		obj->error_callback(error.value());
	} else {
		if( obj->success_callback ) {
			obj->success_callback();
		}
	}
	return 0;
}

Optional<File::Error> ReplayThread::run() {
	StreamOutput stream { &config };

	while( !chThdShouldTerminate() ) {
		if( stream.available() ) {
			auto buffer = stream.get_buffer();
			auto read_result = reader->reader(buffer->data(), buffer->size());
			if( read_result.is_error() ) {
				return read_result.error();
			}
			stream.release_buffer(buffer);
		} else {
			chEvtWaitAny(event_mask_loop_wake);
		}
	}

	return { };
}
