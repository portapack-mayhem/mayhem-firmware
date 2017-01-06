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

#ifndef __MEMORY_MAP_H__
#define __MEMORY_MAP_H__

#include <cstddef>
#include <cstdint>

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#include "utility.hpp"

namespace portapack {
namespace memory {

struct region_t {
public:
	constexpr region_t(
		const uint32_t base,
		const size_t size
	) : base_ { base },
		size_ { size } {

	}

	constexpr uint32_t base() const {
		return base_;
	}

	constexpr uint32_t end() const {
		return base_ + size_;
	}

	constexpr size_t size() const {
		return size_;
	}

private:
	const uint32_t base_;
	const size_t size_;
};

namespace map {

constexpr region_t local_sram_0		{ 0x10000000,  96_KiB };
constexpr region_t local_sram_1		{ 0x10080000,  40_KiB };

constexpr region_t ahb_ram_0		{ 0x20000000,  32_KiB };
constexpr region_t ahb_ram_1		{ 0x20008000,  16_KiB };
constexpr region_t ahb_ram_2		{ 0x2000c000,  16_KiB };

constexpr region_t backup_ram		{ LPC_BACKUP_REG_BASE,        256   };

constexpr region_t spifi_uncached	{ LPC_SPIFI_DATA_BASE,        1_MiB };
constexpr region_t spifi_cached		{ LPC_SPIFI_DATA_CACHED_BASE, spifi_uncached.size() };

/////////////////////////////////

constexpr region_t m4_code			{ local_sram_1.base(), 32_KiB };
constexpr region_t shared_memory	{ m4_code.end(),        8_KiB };

constexpr region_t m4_code_hackrf	= local_sram_0;

} /* namespace map */
} /* namespace memory */
} /* namespace portapack */

#endif/*__MEMORY_MAP_H__*/
