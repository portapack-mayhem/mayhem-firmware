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

#pragma once

#include "file.hpp"

namespace stream {

class Reader {
public:
	virtual File::Result<File::Size> read(void* const buffer, const File::Size bytes) = 0;
	virtual ~Reader() = default;
};

class Writer {
public:
	virtual File::Result<File::Size> write(const void* const buffer, const File::Size bytes) = 0;
	virtual ~Writer() = default;
};

} /* namespace stream */
