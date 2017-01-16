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

#ifndef __PNG_WRITER_H__
#define __PNG_WRITER_H__

#include <cstdint>
#include <cstddef>
#include <string>
#include <array>

#include "ui.hpp"
#include "file.hpp"
#include "crc.hpp"

class PNGWriter {
public:
	~PNGWriter();

	Optional<File::Error> create(const std::filesystem::path& filename);
	
	void write_scanline(const std::array<ui::ColorRGB888, 240>& scanline);

private:
	// TODO: These constants are baked in a few places, do not change blithely.
	static constexpr int width { 240 };
	static constexpr int height { 320 };

	File file { };
	int scanline_count { 0 };
	CRC<32, true, true> crc { 0x04c11db7, 0xffffffff, 0xffffffff };
	Adler32 adler_32 { };

	void write_chunk_header(const size_t length, const std::array<uint8_t, 4>& type);
	void write_chunk_content(const void* const p, const size_t count);

	template<size_t N>
	void write_chunk_content(const std::array<uint8_t, N>& data) {
		write_chunk_content(data.data(), sizeof(data));
	}

	void write_chunk_crc();
	void write_uint32_be(const uint32_t v);
};

#endif/*__PNG_WRITER_H__*/
