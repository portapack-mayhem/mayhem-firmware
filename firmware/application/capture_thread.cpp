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

// BufferExchange ///////////////////////////////////////////////////////////

class BufferExchange {
public:
	BufferExchange(CaptureConfig* const config);
	~BufferExchange();

#if defined(LPC43XX_M0)
	StreamBuffer* get() {
		StreamBuffer* p { nullptr };
		fifo_buffers_for_application->out(p);
		return p;
	}

	bool put(StreamBuffer* const p) {
		return fifo_buffers_for_baseband->in(p);
	}
#endif

#if defined(LPC43XX_M4)
	StreamBuffer* get() {
		StreamBuffer* p { nullptr };
		fifo_buffers_for_baseband->out(p);
		return p;
	}

	bool put(StreamBuffer* const p) {
		return fifo_buffers_for_application->in(p);
	}
#endif

	static FIFO<StreamBuffer*>* fifo_buffers_for_baseband;
	static FIFO<StreamBuffer*>* fifo_buffers_for_application;

private:
	CaptureConfig* const config;
};

FIFO<StreamBuffer*>* BufferExchange::fifo_buffers_for_baseband = nullptr;
FIFO<StreamBuffer*>* BufferExchange::fifo_buffers_for_application = nullptr;

BufferExchange::BufferExchange(
	CaptureConfig* const config
) : config { config }
{
	baseband::capture_start(config);
	fifo_buffers_for_baseband = config->fifo_buffers_empty;
	fifo_buffers_for_application = config->fifo_buffers_full;
}

BufferExchange::~BufferExchange() {
	fifo_buffers_for_baseband = nullptr;
	fifo_buffers_for_application = nullptr;
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
	const auto fifo = BufferExchange::fifo_buffers_for_application;
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
	BufferExchange buffers { &config };

	while( !chThdShouldTerminate() ) {
		auto buffer = buffers.get();
		if( buffer ) {
			auto write_result = writer->write(buffer->data(), buffer->size());
			if( write_result.is_error() ) {
				return write_result.error();
			}
			buffer->empty();
			buffers.put(buffer);
		} else {
			chEvtWaitAny(event_mask_loop_wake);
		}
	}

	return { };
}
