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

#ifndef __SPI_IMAGE_H__
#define __SPI_IMAGE_H__

#include <cstdint>
#include <cstddef>

#include "memory_map.hpp"

namespace portapack {
namespace spi_flash {

struct region_t {
	const size_t offset;
	const size_t size;

	constexpr const void* base() {
		return reinterpret_cast<void*>(portapack::memory::map::spifi_cached.base() + offset);
	}
};

constexpr region_t bootstrap {
	.offset = 0x00000,
	.size = 0x10000,
};

constexpr region_t hackrf {
	.offset = 0x10010,	// Image starts at 0x10 into .dfu file.
	.size = 0x8000,
};

constexpr region_t baseband {
	.offset = 0x20000,
	.size = 0x8000,
};

constexpr region_t application {
	.offset = 0x40000,
	.size = 0x40000,
};

// TODO: Refactor into another header that defines memory regions.
constexpr void* m4_text_ram_base = reinterpret_cast<void*>(0x10080000);

} /* namespace spi_flash */
} /* namespace portapack */

#endif/*__SPI_IMAGE_H__*/
