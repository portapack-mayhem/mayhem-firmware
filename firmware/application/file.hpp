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
#include <array>
#include <memory>
#include <iterator>

class File {
public:
	~File();

	bool open_for_writing(const std::string& file_path);
	bool open_for_reading(const std::string& file_path);
	bool open_for_append(const std::string& file_path);
	bool close();

	bool is_ready();

	bool read(void* const data, const size_t bytes_to_read);
	bool write(const void* const data, const size_t bytes_to_write);

	template<size_t N>
	bool write(const std::array<uint8_t, N>& data) {
		return write(data.data(), N);
	}

	bool puts(const std::string& string);

	bool sync();

private:
	FIL f;
};

namespace std {
namespace filesystem {

using file_status = BYTE;

struct directory_entry : public FILINFO {
	file_status status() const {
		return fattrib;
	}

	const std::string path() const noexcept { return fname; };
};

class directory_iterator {
	struct Impl {
		DIR dir;
		directory_entry filinfo;

		~Impl() {
			f_closedir(&dir);
		}
	};

	std::shared_ptr<Impl> impl;

	friend bool operator!=(const directory_iterator& lhs, const directory_iterator& rhs);

public:
	using difference_type = std::ptrdiff_t;
	using value_type = directory_entry;
	using pointer = const directory_entry*;
	using reference = const directory_entry&;
	using iterator_category = std::input_iterator_tag;

	directory_iterator() noexcept { };
	directory_iterator(const char* path, const char* wild);

	~directory_iterator() { }

	directory_iterator& operator++();

	reference operator*() const {
		// TODO: Exception or assert if impl == nullptr.
		return impl->filinfo;
	}
};

inline const directory_iterator& begin(const directory_iterator& iter) noexcept { return iter; };
inline directory_iterator end(const directory_iterator&) noexcept { return { }; };

inline bool operator!=(const directory_iterator& lhs, const directory_iterator& rhs) { return lhs.impl != rhs.impl; };

bool is_regular_file(const file_status s);

} /* namespace filesystem */
} /* namespace std */

#endif/*__FILE_H__*/
