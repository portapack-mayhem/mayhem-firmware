/*
 * Copyright (C) 2022 José Moreira @cusspvz
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

#pragma once

#include "encoders.hpp"
#include "io.hpp"
#include "cursor.hpp"
#include <functional>
#include <cstdint>
#include <vector>

using namespace encoders;

enum IoBitStreamRepeaterReadType
{
	OOK_READER_COMPLETED = 0,
	OOK_READER_READING_FRAGMENT = 1,
	OOK_READER_READING_PAUSES = 2,
};

class IoBitStreamRepeater : public stream::Reader
{
public:
	// std::function<void(IoBitStreamRepeater &)> on_before_frame_fragment_usage{};
	std::function<void(IoBitStreamRepeater &)> on_complete{};

	IoBitStreamRepeater() = default;

	IoBitStreamRepeater(const IoBitStreamRepeater &) = delete;
	IoBitStreamRepeater &operator=(const IoBitStreamRepeater &) = delete;
	IoBitStreamRepeater &operator=(IoBitStreamRepeater &&) = delete;

	Result<size_t, Error> read(void *const buffer, const size_t bsize) override;
	size_t length();
	void reset();

	std::vector<bool> *frame_fragments{};

	bool completition_requires_pause = false;
	cursor pauses_cursor{0};
	cursor repetitions_cursor{1};
	cursor fragments_cursor{1};

protected:
	size_t bytes_read{0};

private:
	void change_read_type(IoBitStreamRepeaterReadType rt);
	IoBitStreamRepeaterReadType read_type = OOK_READER_READING_FRAGMENT;
};
