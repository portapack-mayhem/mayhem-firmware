/*
 * Copyright (C) 2026 jrlynx13
 * Co-authored with Grok (xAI) via design dialog 2026-05-31
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

#ifndef __TILE_CACHE_H__
#define __TILE_CACHE_H__

#include <cstdint>
#include <cstddef>

#include "bmpfile.hpp"

namespace ui {

class TileCache {
   public:
    static constexpr size_t MAX_SLOTS = 24;

    struct TileKey {
        int8_t zoom;
        int16_t x;
        int16_t y;
    };

    struct CachedTile {
        TileKey key{};
        uint32_t last_access_tick{0};
        bool valid{false};
        BMPFile bmp{};
    };

    TileCache();
    ~TileCache();

    BMPFile* find(const TileKey& key);
    CachedTile* insert_or_replace(const TileKey& key);
    void clear();

   private:
    CachedTile slots_[MAX_SLOTS];
    uint32_t current_tick_{0};

    int find_slot_index(const TileKey& key) const;
    int evict_oldest();
};

}  // namespace ui

#endif
