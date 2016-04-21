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

#include "file.hpp"

#include <algorithm>

File::~File() {
	close();
}

bool File::open_for_writing(const std::string& file_path) {
	const auto open_result = f_open(&f, file_path.c_str(), FA_WRITE | FA_OPEN_ALWAYS);
	return (open_result == FR_OK);
}

bool File::open_for_reading(const std::string& file_path) {
	const auto open_result = f_open(&f, file_path.c_str(), FA_READ | FA_OPEN_EXISTING);
	return (open_result == FR_OK);
}

bool File::open_for_append(const std::string& file_path) {
	if( open_for_writing(file_path) ) {
		const auto seek_result = f_lseek(&f, f_size(&f));
		if( seek_result == FR_OK ) {
			return true;
		} else {
			close();
		}
	}

	return false;
}

bool File::close() {
	f_close(&f);
	return true;
}

bool File::is_ready() {
	return f_error(&f) == 0;
}

bool File::read(void* const data, const size_t bytes_to_read) {
	UINT bytes_read = 0;
	const auto result = f_read(&f, data, bytes_to_read, &bytes_read);
	return (result == FR_OK) && (bytes_read == bytes_to_read);
}

bool File::write(const void* const data, const size_t bytes_to_write) {
	UINT bytes_written = 0;
	const auto result = f_write(&f, data, bytes_to_write, &bytes_written);
	return (result == FR_OK) && (bytes_written == bytes_to_write);
}

bool File::puts(const std::string& string) {
	const auto result = f_puts(string.c_str(), &f);
	return (result >= 0);
}

bool File::sync() {
	const auto result = f_sync(&f);
	return (result == FR_OK);
}

static std::string find_last_file_matching_pattern(const std::string& pattern) {
	std::string last_match;
	for(const auto& entry : std::filesystem::directory_iterator("", pattern.c_str())) {
		if( std::filesystem::is_regular_file(entry.status()) ) {
			const auto match = entry.path();
			if( match > last_match ) {
				last_match = match;
			}
		}
	}
	return last_match;
}

static std::string increment_filename_ordinal(const std::string& filename) {
	std::string result { filename };

	auto it = result.rbegin();

	// Back up past extension.
	for(; it != result.rend(); ++it) {
		if( *it == '.' ) {
			++it;
			break;
		}
	}
	if( it == result.rend() ) {
		return { };
	}

	// Increment decimal number before the extension.
	for(; it != result.rend(); ++it) {
		const auto c = *it;
		if( c < '0' ) {
			return { };
		} else if( c < '9' ) {
			*it += 1;
			break;
		} else if( c == '9' ) {
			*it = '0';
		} else {
			return { };
		}
	}

	return result;
}

std::string next_filename_matching_pattern(const std::string& filename_pattern) {
	auto filename = find_last_file_matching_pattern(filename_pattern);
	if( filename.empty() ) {
		filename = filename_pattern;
		std::replace(std::begin(filename), std::end(filename), '?', '0');
	} else {
		filename = increment_filename_ordinal(filename);
	}
	return filename;
}

namespace std {
namespace filesystem {

directory_iterator::directory_iterator(
	const char* path,
	const char* wild
) {
	impl = std::make_shared<Impl>();
	const auto result = f_findfirst(&impl->dir, &impl->filinfo, path, wild);
	if( result != FR_OK ) {
		impl.reset();
		// TODO: Throw exception if/when I enable exceptions...
	}
}

directory_iterator& directory_iterator::operator++() {
	const auto result = f_findnext(&impl->dir, &impl->filinfo);
	if( (result != FR_OK) || (impl->filinfo.fname[0] == 0) ) {
		impl.reset();
	}
	return *this;
}

bool is_regular_file(const file_status s) {
	return !(s & AM_DIR);
}

} /* namespace filesystem */
} /* namespace std */
