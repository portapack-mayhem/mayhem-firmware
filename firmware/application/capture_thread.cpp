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
#include "buffer_exchange.hpp"

struct BasebandCapture {
	BasebandCapture(CaptureConfig* const config) {
		baseband::capture_start(config);
	}

	~BasebandCapture() {
		baseband::capture_stop();
	}
};

// CaptureThread //////////////////////////////////////////////////////////

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
		chThdWait(thread);
		thread = nullptr;
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
	BasebandCapture capture { &config };
	BufferExchange buffers { &config };

	while( !chThdShouldTerminate() ) {
		auto buffer = buffers.get();
		auto write_result = writer->write(buffer->data(), buffer->size());
		if( write_result.is_error() ) {
			return write_result.error();
		}
		buffer->empty();
		buffers.put(buffer);
	}

	return { };
}
