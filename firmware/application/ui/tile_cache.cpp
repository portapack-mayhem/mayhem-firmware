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

#include "tile_cache.hpp"

namespace ui {

TileCache::TileCache() {
    // slots_ are already default-constructed (BMPFile default ctor is trivial)
}

TileCache::~TileCache() {
    clear();
}

BMPFile* TileCache::find(const TileKey& key) {
    for (size_t i = 0; i < MAX_SLOTS; ++i) {
        if (slots_[i].valid &&
            slots_[i].key.zoom == key.zoom &&
            slots_[i].key.x == key.x &&
            slots_[i].key.y == key.y) {
            slots_[i].last_access_tick = ++current_tick_;
            return &slots_[i].bmp;
        }
    }
    return nullptr;
}

TileCache::CachedTile* TileCache::insert_or_replace(const TileKey& key) {
    int idx = find_slot_index(key);
    if (idx < 0) {
        idx = evict_oldest();
    }

    auto& slot = slots_[idx];
    slot.key = key;
    slot.last_access_tick = ++current_tick_;
    slot.valid = true;

    return &slot;
}

void TileCache::clear() {
    for (auto& slot : slots_) {
        if (slot.valid) {
            slot.bmp.close();
        }
        slot.valid = false;
        slot.last_access_tick = 0;
    }
    current_tick_ = 0;
}

int TileCache::find_slot_index(const TileKey& key) const {
    for (size_t i = 0; i < MAX_SLOTS; ++i) {
        if (slots_[i].valid &&
            slots_[i].key.zoom == key.zoom &&
            slots_[i].key.x == key.x &&
            slots_[i].key.y == key.y) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int TileCache::evict_oldest() {
    int oldest_idx = 0;
    uint32_t oldest_tick = slots_[0].last_access_tick;

    for (size_t i = 1; i < MAX_SLOTS; ++i) {
        if (!slots_[i].valid) {
            return static_cast<int>(i);
        }
        if (slots_[i].last_access_tick < oldest_tick) {
            oldest_tick = slots_[i].last_access_tick;
            oldest_idx = static_cast<int>(i);
        }
    }

    if (slots_[oldest_idx].valid) {
        slots_[oldest_idx].bmp.close();
    }
    return oldest_idx;
}

}  // namespace ui
