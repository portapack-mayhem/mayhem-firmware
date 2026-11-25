/*
 * Copyright (C) 2026 Mayhem firmware contributors
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

/* DAB channel raster for Band III and L-Band, per ETSI EN 300 401 Annex A. */

#ifndef __DAB_BLOCKS_H__
#define __DAB_BLOCKS_H__

#include <cstddef>
#include <cstdint>

namespace dab {

struct Block {
    const char* name;
    uint32_t frequency_hz;
};

struct BlockTable {
    const Block* items;
    size_t count;
};

/* 174-240 MHz, 41 blocks including the national 10N/11N/12N. */
BlockTable band_iii_blocks();

/* 1452-1492 MHz, LA through LW. */
BlockTable l_band_blocks();

/* Both bands. nullptr for an unknown or null name (cache input is unvalidated). */
const Block* find_block(const char* name);

} /* namespace dab */

#endif /*__DAB_BLOCKS_H__*/
