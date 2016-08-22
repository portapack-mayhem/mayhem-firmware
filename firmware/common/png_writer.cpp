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

#include "png_writer.hpp"

static constexpr std::array<uint8_t, 8> png_file_header { {
	0x89, 0x50, 0x4e, 0x47,
	0x0d, 0x0a, 0x1a, 0x0a,
} };

static constexpr std::array<uint8_t, 25> png_ihdr_screen_capture { {
	0x00, 0x00, 0x00, 0x0d,		// IHDR length
	0x49, 0x48, 0x44, 0x52,		// IHDR type
	0x00, 0x00, 0x00, 0xf0,		// width = 240
	0x00, 0x00, 0x01, 0x40,		// height = 320
	0x08,						// bit_depth = 8
	0x02,						// color_type = 2
	0x00,						// compression_method = 0
	0x00,						// filter_method = 0
	0x00,						// interlace_method = 0
	0x0d, 0x8a, 0x66, 0x04,		// CRC
} };

static constexpr std::array<uint8_t, 4> png_idat_chunk_type { {
	0x49, 0x44, 0x41, 0x54,		// IDAT type
} };

static constexpr std::array<uint8_t, 12> png_iend { {
	0x00, 0x00, 0x00, 0x00,		// IEND length
	0x49, 0x45, 0x4e, 0x44,		// IEND type
	0xae, 0x42, 0x60, 0x82,		// CRC
} };

Optional<File::Error> PNGWriter::create(
	const std::filesystem::path& filename
) {
	const auto create_error = file.create(filename);
	if( create_error.is_valid() ) {
		return create_error;
	}

	file.write(png_file_header);
	file.write(png_ihdr_screen_capture);
	
	write_chunk_header(
		2 + height * (5 + 1 + width * 3) + 4,
		png_idat_chunk_type
	);

	constexpr std::array<uint8_t, 2> zlib_header { 0x78, 0x01 };	// Zlib CM, CINFO, FLG.
	write_chunk_content(zlib_header);

	return { };
}

PNGWriter::~PNGWriter() {
	write_chunk_content(adler_32.bytes());
	write_chunk_crc();

	file.write(png_iend);
}

void PNGWriter::write_scanline(const std::array<ui::ColorRGB888, 240>& scanline) {
	constexpr uint8_t scanline_filter_type = 0;
	constexpr uint32_t deflate_block_length = 1 + sizeof(scanline);

	const std::array<uint8_t, 6> deflate_and_scanline_header {
		static_cast<uint8_t>((scanline_count == (height - 1)) ? 0x01 : 0x00),	// DEFLATE header bits, bfinal=0, btype=00
		static_cast<uint8_t>((deflate_block_length >> 0) & 0xff),			// Length LSB
		static_cast<uint8_t>((deflate_block_length >> 8) & 0xff),			// Length MSB
		static_cast<uint8_t>((deflate_block_length >> 0) & 0xff) ^ 0xff,	// ~Length LSB
		static_cast<uint8_t>((deflate_block_length >> 8) & 0xff) ^ 0xff,	// ~Length MSB
		scanline_filter_type,
	};
	write_chunk_content(deflate_and_scanline_header);

	adler_32.feed(scanline_filter_type);
	adler_32.feed(scanline);

	// Small writes to avoid some sort of large-transfer plus block
	// boundary FatFs or SDC driver bug?
	write_chunk_content(&scanline[  0], 80 * sizeof(ui::ColorRGB888));
	write_chunk_content(&scanline[ 80], 80 * sizeof(ui::ColorRGB888));
	write_chunk_content(&scanline[160], 80 * sizeof(ui::ColorRGB888));

	scanline_count++;
}

void PNGWriter::write_chunk_header(
	const size_t length,
	const std::array<uint8_t, 4>& type
) {
	write_uint32_be(length);
	crc.reset();
	write_chunk_content(type);
}

void PNGWriter::write_chunk_content(const void* const p, const size_t count) {
	file.write(p, count);
	crc.process_bytes(p, count);
}

void PNGWriter::write_chunk_crc() {
	write_uint32_be(crc.checksum());
}

void PNGWriter::write_uint32_be(const uint32_t v) {
	file.write(std::array<uint8_t, 4> { {
		static_cast<uint8_t>((v >> 24) & 0xff),
		static_cast<uint8_t>((v >> 16) & 0xff),
		static_cast<uint8_t>((v >>  8) & 0xff),
		static_cast<uint8_t>((v >>  0) & 0xff),
	} });
}
