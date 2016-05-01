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

class Writer {
public:
	virtual bool write(const void* const buffer, const size_t bytes) = 0;
	virtual ~Writer() { };
};

class RawFileWriter : public Writer {
public:
	RawFileWriter(
		const std::string& filename
	) : file { filename, File::openmode::out | File::openmode::binary | File::openmode::trunc }
	{
	}

	bool write(const void* const buffer, const size_t bytes) override {
		return file.write(buffer, bytes);
	}

private:
	File file;
};

class WAVFileWriter : public Writer {
public:
	WAVFileWriter(
		const std::string& filename,
		size_t sampling_rate
	) : file { filename, File::openmode::out | File::openmode::binary | File::openmode::trunc },
		header { sampling_rate }
	{
		update_header();
	}

	~WAVFileWriter() {
		update_header();
	}

	bool write(const void* const buffer, const size_t bytes) override {
		const auto success = file.write(buffer, bytes) ;
		if( success ) {
			bytes_written += bytes;
		}
		return success;
	}

private:
	struct fmt_pcm_t {
		constexpr fmt_pcm_t(
			const uint32_t sampling_rate
		) : nSamplesPerSec { sampling_rate },
			nAvgBytesPerSec { nSamplesPerSec * nBlockAlign }
		{
		}

	private:
		const uint8_t ckID[4] { 'f', 'm', 't', ' ' };
		const uint32_t cksize { 16 };
		const uint16_t wFormatTag { 0x0001 };
		const uint16_t nChannels { 1 };
		const uint32_t nSamplesPerSec;
		const uint32_t nAvgBytesPerSec;
		const uint16_t nBlockAlign { 2 };
		const uint16_t wBitsPerSample { 16 };
	};

	struct data_t {
		void set_size(const uint32_t value) {
			cksize = value;
		}

	private:
		const uint8_t ckID[4] { 'd', 'a', 't', 'a' };
		uint32_t cksize { 0 };
	};

	struct header_t {
		constexpr header_t(
			const uint32_t sampling_rate
		) : fmt { sampling_rate }
		{
		}

		void set_data_size(const uint32_t value) {
			data.set_size(value);
			cksize = sizeof(header_t) + value - 8;
		}

	private:
		const uint8_t riff_id[4] { 'R', 'I', 'F', 'F' };
		uint32_t cksize { 0 };
		const uint8_t wave_id[4] { 'W', 'A', 'V', 'E' };
		fmt_pcm_t fmt;
		data_t data;
	};

	File file;
	header_t header;
	uint64_t bytes_written { 0 };

	void update_header() {
		header.set_data_size(bytes_written);
		const auto old_position = file.seek(0);
		file.write(&header, sizeof(header));
		file.seek(old_position);
	}
};

class StreamOutput {
public:
	StreamOutput(
		CaptureConfig* const config
	) : config { config }
	{
		shared_memory.baseband_queue.push_and_wait(
			CaptureConfigMessage { config }
		);
		fifo = config->fifo;
	}

	~StreamOutput() {
		fifo = nullptr;
		shared_memory.baseband_queue.push_and_wait(
			CaptureConfigMessage { nullptr }
		);
	}

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

class CaptureThread {
public:
	CaptureThread(
		std::unique_ptr<Writer> writer,
		size_t write_size_log2,
		size_t buffer_count_log2
	) : config { write_size_log2, buffer_count_log2 },
		writer { std::move(writer) }
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

	const CaptureConfig& state() const {
		return config;
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
	CaptureConfig config;
	std::unique_ptr<Writer> writer;
	static Thread* thread;

	static msg_t static_fn(void* arg) {
		auto obj = static_cast<CaptureThread*>(arg);
		return obj->run();
	}

	msg_t run() {
		const size_t write_size = 1U << config.write_size_log2;
		const auto write_buffer = std::make_unique<uint8_t[]>(write_size);
		if( !write_buffer ) {
			return false;
		}

		StreamOutput stream { &config };

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
			if( writer->write(write_buffer, write_size) ) {
				success = true;
			}
		}

		led_usb.off();

		return success;
	}
};

#endif/*__CAPTURE_THREAD_H__*/
