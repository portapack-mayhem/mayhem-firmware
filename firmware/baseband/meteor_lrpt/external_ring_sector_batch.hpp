/*
 * Copyright (C) 2026
 *
 * 512-byte sector read-modify-write batching for FatFs-backed deinterleaver rings.
 * Algorithm: collect (index_mod, value) writes, sort by index, merge last-write-wins per index,
 * group by sector, one read+patch+write per touched sector.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef METEOR_LRPT_EXTERNAL_RING_SECTOR_BATCH_HPP
#define METEOR_LRPT_EXTERNAL_RING_SECTOR_BATCH_HPP

#include "external_ring.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <utility>
#include <vector>

namespace meteor_lrpt {

/** Max writes per SatDump read_samples batch (8192) plus slack for marker skips accounting. */
inline constexpr size_t kDeintBatchWriteCap = 16384;

/**
 * Applies batched byte writes to an IMeteorDeintRing using an in-RAM sector scratch `sector_buf`.
 * For each distinct sector touched: read sector via ring (byte-by-byte), patch, write back.
 * When `ring` is RAM-backed this is correct but redundant vs direct writes; use for tests.
 */
inline void apply_sorted_writes_byte_by_byte(
    IMeteorDeintRing& ring,
    std::vector<std::pair<uint32_t, int8_t>>& writes,
    std::array<int8_t, kDeinterleaverSectorBytes>& sector_buf) {
    if (writes.empty())
        return;

    std::sort(writes.begin(), writes.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    /* last-write-wins per index */
    std::vector<std::pair<uint32_t, int8_t>> dedup;
    dedup.reserve(writes.size());
    for (size_t i = 0; i < writes.size();) {
        uint32_t idx = writes[i].first;
        size_t j = i + 1;
        while (j < writes.size() && writes[j].first == idx)
            j++;
        dedup.push_back(writes[j - 1]);
        i = j;
    }

    size_t i = 0;
    while (i < dedup.size()) {
        const uint32_t base = dedup[i].first - (dedup[i].first % kDeinterleaverSectorBytes);
        for (size_t b = 0; b < kDeinterleaverSectorBytes; b++)
            sector_buf[b] = ring.read_byte(base + (uint32_t)b);

        while (i < dedup.size()) {
            const auto& w = dedup[i];
            if (w.first < base || w.first >= base + kDeinterleaverSectorBytes)
                break;
            sector_buf[w.first - base] = w.second;
            i++;
        }
        for (size_t b = 0; b < kDeinterleaverSectorBytes; b++)
            ring.write_byte(base + (uint32_t)b, sector_buf[b]);
    }
}

}  // namespace meteor_lrpt

#endif
