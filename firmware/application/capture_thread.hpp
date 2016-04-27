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

#ifndef __CAPTURE_THREAD_H__
#define __CAPTURE_THREAD_H__

#include "ch.h"

#include "file.hpp"

#include "event_m0.hpp"

#include "portapack_shared_memory.hpp"

#include "hackrf_gpio.hpp"
using namespace hackrf::one;

#include <cstring>

class StreamOutput {
public:
	StreamOutput(
		const size_t write_size_log2,
		const size_t buffer_count_log2
	) : config { write_size_log2, buffer_count_log2 }
	{
		shared_memory.baseband_queue.push_and_wait(
			CaptureConfigMessage { &config }
		);
		fifo = config.fifo;
	}

	~StreamOutput() {
		fifo = nullptr;
		shared_memory.baseband_queue.push_and_wait(
			CaptureConfigMessage { nullptr }
		);
	}

	size_t available() {
		return config.fifo->len();
	}

	size_t read(void* const data, const size_t length) {
		return config.fifo->out(reinterpret_cast<uint8_t*>(data), length);
	}

	static FIFO<uint8_t>* fifo;

private:
	CaptureConfig config;
};

class CaptureThread {
public:
	CaptureThread(
		std::string file_path,
		size_t write_size_log2,
		size_t buffer_count_log2
	) : write_size_log2 { write_size_log2 },
		buffer_count_log2 { buffer_count_log2 },
		file_path { std::move(file_path) }
	{
		// Need significant stack for FATFS
		thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO + 10, CaptureThread::static_fn, this);
	}

	~CaptureThread() {
		if( thread ) {
			chThdTerminate(thread);
			chEvtSignal(thread, EVT_MASK_CAPTURE_THREAD);
			const auto success = chThdWait(thread);
			thread = nullptr;

			if( !success ) {
				led_tx.on();
			}
		}
	}

	static void check_fifo_isr() {
		// TODO: Prevent over-signalling by transmitting a set of 
		// flags from the baseband core.
		const auto fifo = StreamOutput::fifo;
		if( fifo ) {
			chEvtSignalI(thread, EVT_MASK_CAPTURE_THREAD);
		}
	}

private:
	const size_t write_size_log2;
	const size_t buffer_count_log2;
	const std::string file_path;
	File file;
	static Thread* thread;

	static msg_t static_fn(void* arg) {
		auto obj = static_cast<CaptureThread*>(arg);
		return obj->run();
	}

	msg_t run() {
		if( !file.open_for_writing(file_path) ) {
			return false;
		}

		const size_t write_size = 1U << write_size_log2;
		const auto write_buffer = std::make_unique<uint8_t[]>(write_size);
		if( !write_buffer ) {
			return false;
		}

		StreamOutput stream { write_size_log2, buffer_count_log2 };

		while( !chThdShouldTerminate() ) {
			if( stream.available() >= write_size ) {
				if( !transfer(stream, write_buffer.get(), write_size) ) {
					return false; 
				}
			} else {
				chEvtWaitAny(EVT_MASK_CAPTURE_THREAD);
			}
		}

		return true;
	}

	bool transfer(StreamOutput& stream, uint8_t* const write_buffer, const size_t write_size) {
		bool success = false;

		led_usb.on();

		const auto bytes_to_write = stream.read(write_buffer, write_size);
		if( bytes_to_write == write_size ) {
			if( file.write(write_buffer, write_size) ) {
				success = true;
			}
		}

		led_usb.off();

		return success;
	}
};

#endif/*__CAPTURE_THREAD_H__*/
