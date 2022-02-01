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

namespace std {
namespace filesystem {

struct filesystem_error {
	constexpr filesystem_error() = default;

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
	uint32_t err { FR_OK };
};

struct path {
	using string_type = std::u16string;
	using value_type = string_type::value_type;

	static constexpr value_type preferred_separator = u'/';

	path(
	) : _s { }
	{
	}

	path(
		const path& p
	) : _s { p._s }
	{
	}

	path(
		path&& p
	) : _s { std::move(p._s) }
	{
	}

	template<class Source>
	path(
		const Source& source
	) : path { std::begin(source), std::end(source) }
	{
	}

	template<class InputIt>
	path(
		InputIt first,
		InputIt last
	) : _s { first, last }
	{
	}

	path(
		const char16_t* const s
	) : _s { s }
	{
	}

	path(
		const TCHAR* const s
	) : _s { reinterpret_cast<const std::filesystem::path::value_type*>(s) }
	{
	}

	path& operator=(const path& p) {
		_s = p._s;
		return *this;
	}

	path& operator=(path&& p) {
		_s = std::move(p._s);
		return *this;
	}

	path extension() const;
	path filename() const;
	path stem() const;

	bool empty() const {
		return _s.empty();
	}

	const value_type* c_str() const {
		return native().c_str();
	}

	const string_type& native() const {
		return _s;
	}

	std::string string() const;

	path& operator+=(const path& p) {
		_s += p._s;
		return *this;
	}

	path& operator+=(const string_type& str) {
		_s += str;
		return *this;
	}

	path& replace_extension(const path& replacement = path());

private:
	string_type _s;
};

bool operator<(const path& lhs, const path& rhs);
bool operator>(const path& lhs, const path& rhs);

using file_status = BYTE;

static_assert(sizeof(path::value_type) == 2, "sizeof(std::filesystem::path::value_type) != 2");
static_assert(sizeof(path::value_type) == sizeof(TCHAR), "FatFs TCHAR size != std::filesystem::path::value_type");

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

	std::uintmax_t size() const {
		return fsize;
	};

	const std::filesystem::path path() const noexcept { return { fname }; };
};

class directory_iterator {
	struct Impl {
		DIR dir;
		directory_entry filinfo;

		~Impl() {
			f_closedir(&dir);
		}
	};

	std::shared_ptr<Impl> impl { };
	const path pattern { };

	friend bool operator!=(const directory_iterator& lhs, const directory_iterator& rhs);

public:
	using difference_type = std::ptrdiff_t;
	using value_type = directory_entry;
	using pointer = const directory_entry*;
	using reference = const directory_entry&;
	using iterator_category = std::input_iterator_tag;

	directory_iterator() noexcept { };
	directory_iterator(std::filesystem::path path, std::filesystem::path wild);
	
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

bool is_directory(const file_status s);
bool is_regular_file(const file_status s);

space_info space(const path& p);

} /* namespace filesystem */
} /* namespace std */

struct FATTimestamp {
	uint16_t FAT_date;
	uint16_t FAT_time;
};

uint32_t delete_file(const std::filesystem::path& file_path);
uint32_t rename_file(const std::filesystem::path& file_path, const std::filesystem::path& new_name);
FATTimestamp file_created_date(const std::filesystem::path& file_path);
uint32_t make_new_directory(const std::filesystem::path& dir_path);

std::vector<std::filesystem::path> scan_root_files(const std::filesystem::path& directory, const std::filesystem::path& extension);
std::vector<std::filesystem::path> scan_root_directories(const std::filesystem::path& directory);
std::filesystem::path next_filename_stem_matching_pattern(std::filesystem::path filename_stem_pattern);

/* Values added to FatFs FRESULT enum, values outside the FRESULT data type */
static_assert(sizeof(FIL::err) == 1, "FatFs FIL::err size not expected.");

/* Dangerous to expose these, as FatFs native error values are byte-sized. However,
 * my filesystem_error implementation is fine with it. */
#define FR_DISK_FULL	(0x100)
#define FR_EOF          (0x101)
#define FR_BAD_SEEK		(0x102)
#define FR_UNEXPECTED	(0x103)

class File {
public:
	using Size = uint64_t;
	using Offset = uint64_t;
	using Timestamp = uint32_t;
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
	Optional<Error> open(const std::filesystem::path& filename);
	Optional<Error> append(const std::filesystem::path& filename);
	Optional<Error> create(const std::filesystem::path& filename);

	Result<Size> read(void* const data, const Size bytes_to_read);
	Result<Size> write(const void* const data, const Size bytes_to_write);
	
	Result<Offset> seek(const uint64_t Offset);
	Timestamp created_date();
	Size size();

	template<size_t N>
	Result<Size> write(const std::array<uint8_t, N>& data) {
		return write(data.data(), N);
	}

	Optional<Error> write_line(const std::string& s);

	// TODO: Return Result<>.
	Optional<Error> sync();

private:
	FIL f { };

	Optional<Error> open_fatfs(const std::filesystem::path& filename, BYTE mode);
};

#endif/*__FILE_H__*/
