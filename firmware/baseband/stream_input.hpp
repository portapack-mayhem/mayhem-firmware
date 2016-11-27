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

#ifndef __STREAM_INPUT_H__
#define __STREAM_INPUT_H__

#include "message.hpp"
#include "fifo.hpp"

#include <cstdint>
#include <cstddef>
#include <array>
#include <memory>

class StreamInput {
public:
	StreamInput(CaptureConfig* const config);

	StreamInput(const StreamInput&) = delete;
	StreamInput(StreamInput&&) = delete;
	StreamInput& operator=(const StreamInput&) = delete;
	StreamInput& operator=(StreamInput&&) = delete;

	size_t write(const void* const data, const size_t length);

private:
	static constexpr size_t buffer_count_max_log2 = 3;
	static constexpr size_t buffer_count_max = 1U << buffer_count_max_log2;
	
	FIFO<StreamBuffer*> fifo_buffers_empty;
	FIFO<StreamBuffer*> fifo_buffers_full;
	std::array<StreamBuffer, buffer_count_max> buffers { };
	std::array<StreamBuffer*, buffer_count_max> buffers_empty { };
	std::array<StreamBuffer*, buffer_count_max> buffers_full { };
	StreamBuffer* active_buffer { nullptr };
	CaptureConfig* const config { nullptr };
	std::unique_ptr<uint8_t[]> data { };
};

#endif/*__STREAM_INPUT_H__*/
