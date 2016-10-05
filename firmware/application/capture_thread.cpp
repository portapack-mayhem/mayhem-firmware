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

#include "capture_thread.hpp"

#include "baseband_api.hpp"

// StreamOutput ///////////////////////////////////////////////////////////

class StreamOutput {
public:
	StreamOutput(CaptureConfig* const config);
	~StreamOutput();

	StreamBuffer* get_empty() {
		StreamBuffer* p { nullptr };
		fifo_buffers_empty->out(p);
		return p;
	}

	bool put_empty(StreamBuffer* const p) {
		return fifo_buffers_empty->in(p);
	}

	StreamBuffer* get_full() {
		StreamBuffer* p { nullptr };
		fifo_buffers_full->out(p);
		return p;
	}

	bool put_full(StreamBuffer* const p) {
		return fifo_buffers_full->in(p);
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

Thread* CaptureThread::thread = nullptr;

CaptureThread::CaptureThread(
	std::unique_ptr<stream::Writer> writer,
	size_t write_size,
	size_t buffer_count,
	std::function<void()> success_callback,
	std::function<void(File::Error)> error_callback
) : config { write_size, buffer_count },
	writer { std::move(writer) },
	success_callback { std::move(success_callback) },
	error_callback { std::move(error_callback) }
{
	// Need significant stack for FATFS
	thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO + 10, CaptureThread::static_fn, this);
}

CaptureThread::~CaptureThread() {
	if( thread ) {
		chThdTerminate(thread);
		chEvtSignal(thread, event_mask_loop_wake);
		chThdWait(thread);
		thread = nullptr;
	}
}

void CaptureThread::check_fifo_isr() {
	// TODO: Prevent over-signalling by transmitting a set of 
	// flags from the baseband core.
	const auto fifo = StreamOutput::fifo_buffers_full;
	if( fifo ) {
		if( !fifo->is_empty() ) {
			chEvtSignalI(thread, event_mask_loop_wake);
		}
	}
}

msg_t CaptureThread::static_fn(void* arg) {
	auto obj = static_cast<CaptureThread*>(arg);
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

Optional<File::Error> CaptureThread::run() {
	StreamOutput stream { &config };

	while( !chThdShouldTerminate() ) {
		auto buffer = stream.get_full();
		if( buffer ) {
			auto write_result = writer->write(buffer->data(), buffer->size());
			if( write_result.is_error() ) {
				return write_result.error();
			}
			buffer->empty();
			stream.put_empty(buffer);
		} else {
			chEvtWaitAny(event_mask_loop_wake);
		}
	}

	return { };
}
