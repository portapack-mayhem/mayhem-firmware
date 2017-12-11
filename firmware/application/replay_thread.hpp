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

#include "io.hpp"
#include "optional.hpp"

#include <cstdint>
#include <cstddef>
#include <utility>

class ReplayThread {
public:
	ReplayThread(
		std::unique_ptr<stream::Reader> reader,
		size_t read_size,
		size_t buffer_count,
		bool* ready_signal,
		std::function<void(uint32_t return_code)> terminate_callback
	);
	~ReplayThread();

	ReplayThread(const ReplayThread&) = delete;
	ReplayThread(ReplayThread&&) = delete;
	ReplayThread& operator=(const ReplayThread&) = delete;
	ReplayThread& operator=(ReplayThread&&) = delete;

	const ReplayConfig& state() const {
		return config;
	};

	enum replaythread_return {
		READ_ERROR = 0,
		END_OF_FILE,
		TERMINATED
	};

private:
	ReplayConfig config;
	std::unique_ptr<stream::Reader> reader;
	bool* ready_sig;
	std::function<void(uint32_t return_code)> terminate_callback;
	Thread* thread { nullptr };

	static msg_t static_fn(void* arg);

	uint32_t run();
};

#endif/*__REPLAY_THREAD_H__*/
