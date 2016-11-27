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

#include "event_m0.hpp"

#include "io.hpp"
#include "optional.hpp"

#include <cstdint>
#include <cstddef>
#include <utility>

class CaptureThread {
public:
	CaptureThread(
		std::unique_ptr<stream::Writer> writer,
		size_t write_size,
		size_t buffer_count,
		std::function<void()> success_callback,
		std::function<void(File::Error)> error_callback
	);
	~CaptureThread();

	CaptureThread(const CaptureThread&) = delete;
	CaptureThread(CaptureThread&&) = delete;
	CaptureThread& operator=(const CaptureThread&) = delete;
	CaptureThread& operator=(CaptureThread&&) = delete;

	const CaptureConfig& state() const {
		return config;
	}

private:
	CaptureConfig config;
	std::unique_ptr<stream::Writer> writer;
	std::function<void()> success_callback;
	std::function<void(File::Error)> error_callback;
	Thread* thread { nullptr };

	static msg_t static_fn(void* arg);

	Optional<File::Error> run();
};

#endif/*__CAPTURE_THREAD_H__*/
