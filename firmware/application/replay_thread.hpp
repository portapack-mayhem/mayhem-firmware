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

#ifndef __REPLAY_THREAD_H__
#define __REPLAY_THREAD_H__

#include "ch.h"

#include "event_m0.hpp"

#include "file.hpp"
#include "optional.hpp"

#include <cstdint>
#include <cstddef>
#include <utility>

class Reader {
public:
	virtual File::Result<size_t> read(const void* const buffer, const size_t bytes) = 0;
	virtual ~Reader() = default;
};

class ReplayThread {
public:
	ReplayThread(
		std::unique_ptr<Reader> reader,
		size_t read_size,
		size_t buffer_count,
		std::function<void()> success_callback,
		std::function<void(File::Error)> error_callback
	);
	~ReplayThread();

	const ReplayConfig& state() const {
		return config;
	}

	static void check_fifo_isr();

private:
	static constexpr auto event_mask_loop_wake = EVENT_MASK(0);

	ReplayConfig config;
	std::unique_ptr<Reader> reader;
	std::function<void()> success_callback;
	std::function<void(File::Error)> error_callback;
	static Thread* thread;

	static msg_t static_fn(void* arg);

	Optional<File::Error> run();
};

#endif/*__REPLAY_THREAD_H__*/
