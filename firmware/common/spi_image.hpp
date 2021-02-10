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

#ifndef __SPI_IMAGE_H__
#define __SPI_IMAGE_H__

#include <cstdint>
#include <cstddef>

#include "memory_map.hpp"

extern uint32_t _textend;

namespace portapack {
namespace spi_flash {

struct image_tag_t {
	constexpr image_tag_t(
	) : c { 0, 0, 0, 0 }
	{
	}

	constexpr image_tag_t(
		char c0, char c1, char c2, char c3
	) : c { c0, c1, c2, c3 }
	{
	}

	image_tag_t& operator=(const image_tag_t& other) {
		c[0] = other.c[0];
		c[1] = other.c[1];
		c[2] = other.c[2];
		c[3] = other.c[3];
		return *this;
	}

	bool operator==(const image_tag_t& other) const {
		return (c[0] == other.c[0]) && (c[1] == other.c[1]) && (c[2] == other.c[2]) && (c[3] == other.c[3]);
	}

	operator bool() const {
		return (c[0] != 0) || (c[1] != 0) || (c[2] != 0) || (c[3] != 0);
	}

private:
	char c[4];
};

constexpr image_tag_t image_tag_acars				{ 'P', 'A', 'C', 'A' };
constexpr image_tag_t image_tag_adsb_rx				{ 'P', 'A', 'D', 'R' };
constexpr image_tag_t image_tag_afsk_rx				{ 'P', 'A', 'F', 'R' };
constexpr image_tag_t image_tag_btle_rx				{ 'P', 'B', 'T', 'R' };
constexpr image_tag_t image_tag_nrf_rx				{ 'P', 'N', 'R', 'R' };
constexpr image_tag_t image_tag_ais					{ 'P', 'A', 'I', 'S' };
constexpr image_tag_t image_tag_am_audio			{ 'P', 'A', 'M', 'A' };
constexpr image_tag_t image_tag_am_tv			        { 'P', 'A', 'M', 'T' };
constexpr image_tag_t image_tag_capture				{ 'P', 'C', 'A', 'P' };
constexpr image_tag_t image_tag_ert					{ 'P', 'E', 'R', 'T' };
constexpr image_tag_t image_tag_nfm_audio			{ 'P', 'N', 'F', 'M' };
constexpr image_tag_t image_tag_pocsag				{ 'P', 'P', 'O', 'C' };
constexpr image_tag_t image_tag_sonde				{ 'P', 'S', 'O', 'N' };
constexpr image_tag_t image_tag_tpms				{ 'P', 'T', 'P', 'M' };
constexpr image_tag_t image_tag_wfm_audio			{ 'P', 'W', 'F', 'M' };
constexpr image_tag_t image_tag_wideband_spectrum	{ 'P', 'S', 'P', 'E' };
constexpr image_tag_t image_tag_test				{ 'P', 'T', 'S', 'T' };

constexpr image_tag_t image_tag_adsb_tx				{ 'P', 'A', 'D', 'T' };
constexpr image_tag_t image_tag_afsk				{ 'P', 'A', 'F', 'T' };
constexpr image_tag_t image_tag_audio_tx			{ 'P', 'A', 'T', 'X' };
constexpr image_tag_t image_tag_fsktx				{ 'P', 'F', 'S', 'K' };
constexpr image_tag_t image_tag_jammer				{ 'P', 'J', 'A', 'M' };
constexpr image_tag_t image_tag_mic_tx				{ 'P', 'M', 'T', 'X' };
constexpr image_tag_t image_tag_ook					{ 'P', 'O', 'O', 'K' };
constexpr image_tag_t image_tag_rds					{ 'P', 'R', 'D', 'S' };
constexpr image_tag_t image_tag_replay				{ 'P', 'R', 'E', 'P' };
constexpr image_tag_t image_tag_gps					{ 'P', 'G', 'P', 'S' };
constexpr image_tag_t image_tag_siggen				{ 'P', 'S', 'I', 'G' };
constexpr image_tag_t image_tag_sstv_tx				{ 'P', 'S', 'T', 'X' };
constexpr image_tag_t image_tag_tones				{ 'P', 'T', 'O', 'N' };

constexpr image_tag_t image_tag_noop				{ 'P', 'N', 'O', 'P' };

constexpr image_tag_t image_tag_hackrf				{ 'H', 'R', 'F', '1' };

struct chunk_t {
	const image_tag_t tag;
	const uint32_t length;
	const uint8_t data[];

	const chunk_t* next() const {
		return reinterpret_cast<const chunk_t*>(&data[length]);
	}
};

struct region_t {
	const size_t offset;
	const size_t size;

	constexpr const void* base() const {
		return reinterpret_cast<void*>(portapack::memory::map::spifi_cached.base() + offset);
	}
};

const region_t images {
	.offset = reinterpret_cast<uint32_t>(&_textend),
	.size = portapack::memory::map::spifi_cached.size() - reinterpret_cast<uint32_t>(&_textend),
};

const region_t application {
	.offset = 0x00000,
	.size = reinterpret_cast<uint32_t>(&_textend),
};

} /* namespace spi_flash */
} /* namespace portapack */

#endif/*__SPI_IMAGE_H__*/
