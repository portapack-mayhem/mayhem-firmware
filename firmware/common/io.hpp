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

#include "result.hpp"
#include "error.hpp"

namespace stream
{

	const size_t BASE_BLOCK_SIZE = 4096;

	class Reader
	{
	public:
		virtual Result<size_t> read(void *const buffer, const size_t bytes) = 0;
		virtual ~Reader() = default;

		Result<size_t> fully_read(void *p, const size_t count);
		Result<size_t> fully_read_up_to_max_iterations(void *p, const size_t count, const size_t max_zero_iterations);
	};

	class Writer
	{
	public:
		virtual Result<size_t> write(const void *const buffer, const size_t bytes) = 0;
		virtual ~Writer() = default;

		Result<size_t> fully_write(const void *p, const size_t count);
		Result<size_t> fully_write_up_to_max_iterations(const void *p, const size_t count, const size_t max_zero_iterations);
	};

	class Duplex : public Reader, public Writer
	{
	};

	// Think on a better way to introduce stream pipelines into the mix.
	// we can later discard the thread-based stream reader/writer and have a centralized mechanism to handle
	// all the stream pipelines.
	// class Duplex : public Reader, Writer
	// {
	// public:
	// 	virtual private Result<size_t> transform(const void *const buffer, const size_t bytes) = 0;
	//  void pipe(Writer& writer)
	//  void unpipe(Writer& writer)
	// };

} /* namespace stream */
