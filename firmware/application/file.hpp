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
#include <cstdint>
#include <string>
#include <array>
#include <memory>
#include <iterator>

std::string next_filename_stem_matching_pattern(const std::string& filename_stem_pattern);

namespace std {
namespace filesystem {

struct filesystem_error {
	const uint32_t err;

	std::string what() const;
};

using path = std::string;
using file_status = BYTE;

struct space_info {
	static_assert(sizeof(std::uintmax_t) >= 8, "std::uintmax_t too small (<uint64_t)");

	std::uintmax_t capacity;
	std::uintmax_t free;
	std::uintmax_t available;
};

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

space_info space(const path& p);

} /* namespace filesystem */
} /* namespace std */

class File {
public:
	enum openmode {
		app = 0x100,
		binary = 0x200,
		in = FA_READ,
		out = FA_WRITE,
		trunc = FA_CREATE_ALWAYS,
		ate = FA_OPEN_ALWAYS,
	};

	File(const std::string& filename, openmode mode);
	~File();

	bool is_open() const {
		return err == FR_OK;
	}

	bool bad() const {
		return err != FR_OK;
	}

	std::filesystem::filesystem_error error() const {
		return { err };
	}

	bool read(void* const data, const size_t bytes_to_read);
	bool write(const void* const data, const size_t bytes_to_write);

	uint64_t seek(const uint64_t new_position);

	template<size_t N>
	bool write(const std::array<uint8_t, N>& data) {
		return write(data.data(), N);
	}

	bool puts(const std::string& string);

	bool sync();

private:
	FIL f;
	uint32_t err;
};

inline constexpr File::openmode operator|(File::openmode a, File::openmode b) {
	return File::openmode(static_cast<int>(a) | static_cast<int>(b));
}

#endif/*__FILE_H__*/
