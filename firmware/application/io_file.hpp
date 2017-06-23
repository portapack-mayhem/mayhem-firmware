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

#include "io.hpp"

#include "file.hpp"
#include "optional.hpp"

#include <cstdint>

class FileReader : public stream::Reader {
public:
	FileReader() = default;

	FileReader(const FileReader&) = delete;
	FileReader& operator=(const FileReader&) = delete;
	FileReader(FileReader&& file) = delete;
	FileReader& operator=(FileReader&&) = delete;

	Optional<File::Error> open(const std::filesystem::path& filename) {
		return file.open(filename);
	}
	
	File::Result<File::Size> read(void* const buffer, const File::Size bytes) override;
	
protected:
	File file { };
	uint64_t bytes_read { 0 };
};

class FileWriter : public stream::Writer {
public:
	FileWriter() = default;

	FileWriter(const FileWriter&) = delete;
	FileWriter& operator=(const FileWriter&) = delete;
	FileWriter(FileWriter&& file) = delete;
	FileWriter& operator=(FileWriter&&) = delete;

	Optional<File::Error> create(const std::filesystem::path& filename) {
		return file.create(filename);
	}

	File::Result<File::Size> write(const void* const buffer, const File::Size bytes) override;
	
protected:
	File file { };
	uint64_t bytes_written { 0 };
};

using RawFileWriter = FileWriter;
