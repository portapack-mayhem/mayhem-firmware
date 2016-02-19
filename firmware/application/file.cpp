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

File::~File() {
	close();
}

bool File::open(const std::string& file_path) {
	const auto open_result = f_open(&f, file_path.c_str(), FA_WRITE | FA_OPEN_ALWAYS);
	return (open_result == FR_OK);
}

bool File::open_for_append(const std::string& file_path) {
	if( open(file_path) ) {
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
