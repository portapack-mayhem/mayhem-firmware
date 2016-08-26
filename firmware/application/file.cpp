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

#include "file.hpp"

#include <algorithm>

/* Values added to FatFs FRESULT enum, values outside the FRESULT data type */
static_assert(sizeof(FIL::err) == 1, "FatFs FIL::err size not expected.");
#define FR_DISK_FULL	(0x100)
#define FR_EOF          (0x101)
#define FR_BAD_SEEK		(0x102)
#define FR_UNEXPECTED	(0x103)

Optional<File::Error> File::open_fatfs(const std::string& filename, BYTE mode) {
	auto result = f_open(&f, filename.c_str(), mode);
	if( result == FR_OK ) {
		if( mode & FA_OPEN_ALWAYS ) {
			const auto result = f_lseek(&f, f_size(&f));
			if( result != FR_OK ) {
				f_close(&f);
			}
		}
	}

	if( result == FR_OK ) {
		return { };
	} else {
		return { result };
	}
}

Optional<File::Error> File::open(const std::string& filename) {
	return open_fatfs(filename, FA_READ);
}

Optional<File::Error> File::append(const std::string& filename) {
	return open_fatfs(filename, FA_WRITE | FA_OPEN_ALWAYS);
}

Optional<File::Error> File::create(const std::string& filename) {
	return open_fatfs(filename, FA_WRITE | FA_CREATE_ALWAYS);
}

File::~File() {
	f_close(&f);
}

File::Result<size_t> File::read(void* const data, const size_t bytes_to_read) {
	UINT bytes_read = 0;
	const auto result = f_read(&f, data, bytes_to_read, &bytes_read);
	if( result == FR_OK ) {
		return { static_cast<size_t>(bytes_read) };
	} else {
		return { static_cast<Error>(result) };
	}
}

File::Result<size_t> File::write(const void* const data, const size_t bytes_to_write) {
	UINT bytes_written = 0;
	const auto result = f_write(&f, data, bytes_to_write, &bytes_written);
	if( result == FR_OK ) {
		if( bytes_to_write == bytes_written ) {
			return { static_cast<size_t>(bytes_written) };
		} else {
			return Error { FR_DISK_FULL };
		}
	} else {
		return { static_cast<Error>(result) };
	}
}

File::Result<uint64_t> File::seek(const uint64_t new_position) {
	/* NOTE: Returns *old* position, not new position */
	const auto old_position = f_tell(&f);
	const auto result = f_lseek(&f, new_position);
	if( result != FR_OK ) {
		return { static_cast<Error>(result) };
	}
	if( f_tell(&f) != new_position ) {
		return { static_cast<Error>(FR_BAD_SEEK) };
	}
	return { static_cast<uint64_t>(old_position) };
}

Optional<File::Error> File::write_line(const std::string& s) {
	const auto result_s = write(s.c_str(), s.size());
	if( result_s.is_error() ) {
		return { result_s.error() };
	}

	const auto result_crlf = write("\r\n", 2);
	if( result_crlf.is_error() ) {
		return { result_crlf.error() };
	}

	return { };
}

Optional<File::Error> File::sync() {
	const auto result = f_sync(&f);
	if( result == FR_OK ) {
		return { };
	} else {
		return { result };
	}
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

std::string remove_filename_extension(const std::string& filename) {
	const auto extension_index = filename.find_last_of('.');
	return filename.substr(0, extension_index);
}

static std::string increment_filename_stem_ordinal(const std::string& filename_stem) {
	std::string result { filename_stem };

	auto it = result.rbegin();

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

std::string next_filename_stem_matching_pattern(const std::string& filename_stem_pattern) {
	const auto filename = find_last_file_matching_pattern(filename_stem_pattern + ".*");
	auto filename_stem = remove_filename_extension(filename);
	if( filename_stem.empty() ) {
		filename_stem = filename_stem_pattern;
		std::replace(std::begin(filename_stem), std::end(filename_stem), '?', '0');
	} else {
		filename_stem = increment_filename_stem_ordinal(filename_stem);
	}
	return filename_stem;
}

std::vector<std::string> scan_root_files(const std::string& directory, const std::string& extension) {
	std::vector<std::string> file_list { };
	std::string fname;
	FRESULT res;
	DIR dir;
	static FILINFO fno;

	res = f_opendir(&dir, directory.c_str());
	if (res == FR_OK) {
		for (;;) {
			res = f_readdir(&dir, &fno);
			if (res != FR_OK || fno.fname[0] == 0) break;
			fname.assign(fno.fname);
			if (fname.find(extension) != std::string::npos)
				file_list.push_back(fname);
		}
		f_closedir(&dir);
	}
	
	return file_list;
}

namespace std {
namespace filesystem {

std::string filesystem_error::what() const {
	switch(err) {
	case FR_OK: 					return "ok";
	case FR_DISK_ERR:				return "disk error";
	case FR_INT_ERR:				return "insanity detected";
	case FR_NOT_READY:				return "not ready";
	case FR_NO_FILE:				return "no file";
	case FR_NO_PATH:				return "no path";
	case FR_INVALID_NAME:			return "invalid name";
	case FR_DENIED:					return "denied";
	case FR_EXIST:					return "exists";
	case FR_INVALID_OBJECT:			return "invalid object";
	case FR_WRITE_PROTECTED:		return "write protected";
	case FR_INVALID_DRIVE:			return "invalid drive";
	case FR_NOT_ENABLED:			return "not enabled";
	case FR_NO_FILESYSTEM:			return "no filesystem";
	case FR_MKFS_ABORTED:			return "mkfs aborted";
	case FR_TIMEOUT:				return "timeout";
	case FR_LOCKED:					return "locked";
	case FR_NOT_ENOUGH_CORE:		return "not enough core";
	case FR_TOO_MANY_OPEN_FILES:	return "too many open files";
	case FR_INVALID_PARAMETER:		return "invalid parameter";
	case FR_EOF:					return "end of file";
	case FR_DISK_FULL:				return "disk full";
	case FR_BAD_SEEK:				return "bad seek";
	case FR_UNEXPECTED:				return "unexpected";
	default:						return "unknown";
	}
}

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

space_info space(const path& p) {
	DWORD free_clusters { 0 };
	FATFS* fs;
	if( f_getfree(p.c_str(), &free_clusters, &fs) == FR_OK ) {
#if _MAX_SS != _MIN_SS
		static_assert(false, "FatFs not configured for fixed sector size");
#else
		const std::uintmax_t cluster_bytes = fs->csize * _MIN_SS;
		return {
			(fs->n_fatent - 2) * cluster_bytes,
			free_clusters * cluster_bytes,
			free_clusters * cluster_bytes,
		};
#endif
	} else {
		return { 0, 0, 0 };
	}
}

} /* namespace filesystem */
} /* namespace std */
