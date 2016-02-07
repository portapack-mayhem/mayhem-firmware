/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __FILE_H__
#define __FILE_H__

#include "ff.h"

#include <cstddef>
#include <string>

class File {
public:
	~File();

	bool open_for_append(const std::string& file_path);
	bool close();

	bool is_ready();

	bool read(void* const data, const size_t bytes_to_read);
	bool write(const void* const data, const size_t bytes_to_write);

	bool puts(const std::string& string);

	bool sync();

private:
	const std::string file_path;
	
	FIL f;
};

#endif/*__FILE_H__*/
