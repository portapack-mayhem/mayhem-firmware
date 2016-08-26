/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __FILE_H__
#define __FILE_H__

#include "ff.h"

#include "optional.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <array>
#include <memory>
#include <iterator>
#include <vector>

std::string next_filename_stem_matching_pattern(const std::string& filename_stem_pattern);
std::vector<std::string> scan_root_files(const std::string& extension);

namespace std {
namespace filesystem {

struct filesystem_error {
	constexpr filesystem_error(
	) : err { FR_OK }
	{
	}

	constexpr filesystem_error(
		FRESULT fatfs_error
	) : err { fatfs_error }
	{
	}

	constexpr filesystem_error(
		unsigned int other_error
	) : err { other_error }
	{
	}

	uint32_t code() const {
		return err;
	}
	
	std::string what() const;

private:
	uint32_t err;
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
	using Error = std::filesystem::filesystem_error;

	template<typename T>
	struct Result {
		enum class Type {
			Success,
			Error,
		} type;
		union {
			T value_;
			Error error_;
		};

		bool is_ok() const {
			return type == Type::Success;
		}

		bool is_error() const {
			return type == Type::Error;
		}

		const T& value() const {
			return value_;
		}

		Error error() const {
			return error_;
		}

		Result() = delete;

		constexpr Result(
			T value
		) : type { Type::Success },
			value_ { value }
		{
		}

		constexpr Result(
			Error error
		) : type { Type::Error },
			error_ { error }
		{
		}

		~Result() {
			if( type == Type::Success ) {
				value_.~T();
			}
		}
	};

	File() { };
	~File();

	/* Prevent copies */
	File(const File&) = delete;
	File& operator=(const File&) = delete;

	// TODO: Return Result<>.
	Optional<Error> open(const std::string& filename);
	Optional<Error> append(const std::string& filename);
	Optional<Error> create(const std::string& filename);

	Result<size_t> read(void* const data, const size_t bytes_to_read);
	Result<size_t> write(const void* const data, const size_t bytes_to_write);

	Result<uint64_t> seek(const uint64_t new_position);

	template<size_t N>
	Result<size_t> write(const std::array<uint8_t, N>& data) {
		return write(data.data(), N);
	}

	Optional<Error> write_line(const std::string& s);

	// TODO: Return Result<>.
	Optional<Error> sync();

private:
	FIL f;

	Optional<Error> open_fatfs(const std::string& filename, BYTE mode);
};

#endif/*__FILE_H__*/
