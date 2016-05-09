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
		return fifo->len();
	}

	size_t read(void* const data, const size_t length) {
		return fifo->out(reinterpret_cast<uint8_t*>(data), length);
	}

	static FIFO<uint8_t>* fifo;

private:
	CaptureConfig* const config;
};

FIFO<uint8_t>* StreamOutput::fifo = nullptr;

StreamOutput::StreamOutput(
	CaptureConfig* const config
) : config { config }
{
	shared_memory.baseband_queue.push_and_wait(
		CaptureConfigMessage { config }
	);
	fifo = config->fifo;
}

StreamOutput::~StreamOutput() {
	fifo = nullptr;
	shared_memory.baseband_queue.push_and_wait(
		CaptureConfigMessage { nullptr }
	);
}

// CaptureThread //////////////////////////////////////////////////////////

Thread* CaptureThread::thread = nullptr;

CaptureThread::CaptureThread(
	std::unique_ptr<Writer> writer,
	size_t write_size_log2,
	size_t buffer_count_log2
) : config { write_size_log2, buffer_count_log2 },
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
	const auto fifo = StreamOutput::fifo;
	if( fifo ) {
		chEvtSignalI(thread, EVT_MASK_CAPTURE_THREAD);
	}
}

msg_t CaptureThread::run() {
	const size_t write_size = 1U << config.write_size_log2;
	const auto write_buffer = std::make_unique<uint8_t[]>(write_size);
	if( !write_buffer ) {
		return false;
	}

	StreamOutput stream { &config };

	while( !chThdShouldTerminate() ) {
		if( stream.available() >= write_size ) {
			if( stream.read(write_buffer.get(), write_size) != write_size ) {
				return false;
			}
			if( !writer->write(write_buffer.get(), write_size) ) {
				return false;
			}
		} else {
			chEvtWaitAny(EVT_MASK_CAPTURE_THREAD);
		}
	}

	return true;
}
