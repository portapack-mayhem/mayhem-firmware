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
	StreamOutput() {
		shared_memory.baseband_queue.push_and_wait(
			CaptureConfigMessage { &config }
		);
	}

	~StreamOutput() {
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

private:
	CaptureConfig config;
};

class CaptureThread {
public:
	CaptureThread(
		std::string file_path
	) : file_path { std::move(file_path) },
		write_buffer { std::make_unique<std::array<uint8_t, write_size>>() }
	{
		// Need significant stack for FATFS
		thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO + 10, CaptureThread::static_fn, this);
	}

	~CaptureThread() {
		const auto thread_tmp = thread;

		if( thread_tmp ) {
			thread = nullptr;
			chThdTerminate(thread_tmp);
			chEvtSignal(thread_tmp, EVT_FIFO_HIGHWATER);
			const auto success = chThdWait(thread_tmp);

			if( !success ) {
				led_tx.on();
			}
		}
	}

	static void check_fifo_isr() {
		// TODO: Prevent over-signalling by transmitting a set of 
		// flags from the baseband core.
		if( thread ) {
			chEvtSignalI(thread, EVT_FIFO_HIGHWATER);
		}
	}

private:
	static constexpr size_t write_size = 16384;
	static constexpr eventmask_t EVT_FIFO_HIGHWATER = 1;

	const std::string file_path;
	std::unique_ptr<std::array<uint8_t, write_size>> write_buffer;
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

		StreamOutput stream;

		while( !chThdShouldTerminate() ) {
			chEvtWaitAny(EVT_FIFO_HIGHWATER);

			while( stream.available() >= write_buffer->size() ) {
				if( !transfer(stream, write_buffer.get()) ) {
					return false; 
				}
			}
		}

		return true;
	}

	bool transfer(StreamOutput& stream, std::array<uint8_t, write_size>* const write_buffer) {
		bool success = false;

		led_usb.on();

		const auto bytes_to_write = stream.read(write_buffer->data(), write_buffer->size());
		if( bytes_to_write == write_buffer->size() ) {
			if( file.write(write_buffer->data(), write_buffer->size()) ) {
				success = true;
			}
		}

		led_usb.off();

		return success;
	}
};

#endif/*__CAPTURE_THREAD_H__*/
