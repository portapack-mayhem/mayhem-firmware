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

#include "portapack_shared_memory.hpp"

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
	shared_memory.baseband_queue.push_and_wait(
		CaptureConfigMessage { config }
	);
	fifo_buffers_empty = config->fifo_buffers_empty;
	fifo_buffers_full = config->fifo_buffers_full;
}

StreamOutput::~StreamOutput() {
	fifo_buffers_full = nullptr;
	fifo_buffers_empty = nullptr;
	shared_memory.baseband_queue.push_and_wait(
		CaptureConfigMessage { nullptr }
	);
}

// CaptureThread //////////////////////////////////////////////////////////

Thread* CaptureThread::thread = nullptr;

CaptureThread::CaptureThread(
	std::unique_ptr<Writer> writer,
	size_t write_size,
	size_t buffer_count
) : config { write_size, buffer_count },
	writer { std::move(writer) }
{
	// Need significant stack for FATFS
	thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO + 10, CaptureThread::static_fn, this);
}

CaptureThread::~CaptureThread() {
	if( thread ) {
		chThdTerminate(thread);
		chEvtSignal(thread, EVT_MASK_CAPTURE_THREAD);
		chThdWait(thread);
		thread = nullptr;
	}
}

void CaptureThread::check_fifo_isr() {
	// TODO: Prevent over-signalling by transmitting a set of 
	// flags from the baseband core.
	const auto fifo = StreamOutput::fifo_buffers_full;
	if( fifo ) {
		chEvtSignalI(thread, EVT_MASK_CAPTURE_THREAD);
	}
}

msg_t CaptureThread::run() {
	StreamOutput stream { &config };

	while( !chThdShouldTerminate() ) {
		if( stream.available() ) {
			auto buffer = stream.get_buffer();
			if( !writer->write(buffer->data(), buffer->size()) ) {
				return false;
			}
			stream.release_buffer(buffer);
		} else {
			chEvtWaitAny(EVT_MASK_CAPTURE_THREAD);
		}
	}

	return true;
}
