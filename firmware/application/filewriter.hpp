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

#ifndef __FILEWRITER_H__
#define __FILEWRITER_H__

#include "ui_widget.hpp"

#include "capture_thread.hpp"
#include "signal.hpp"
#include "file.hpp"

#include <cstddef>
#include <string>
#include <memory>

namespace ui {
	
class FileWriter : public Writer {
public:
	FileWriter() = default;

	FileWriter(const FileWriter&) = delete;
	FileWriter& operator=(const FileWriter&) = delete;
	FileWriter(FileWriter&& file) = delete;
	FileWriter& operator=(FileWriter&&) = delete;

	Optional<File::Error> create(const std::string& filename);

	File::Result<size_t> write(const void* const buffer, const size_t bytes) override;

protected:
	File file;
	uint64_t bytes_written { 0 };
};

} /* namespace ui */

#endif/*__FILEWRITER_H__*/
