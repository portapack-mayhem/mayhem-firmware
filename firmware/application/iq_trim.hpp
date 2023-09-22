/*
 * Copyright (C) 2023 Kyle Reed
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

#ifndef __IQ_TRIM_H__
#define __IQ_TRIM_H__

#include "complex.hpp"
#include "file.hpp"
#include "optional.hpp"

#include <functional>
#include <limits>

namespace iq {

/* Data needed to trim a capture by sample range. */
struct TrimRange {
    uint64_t start_sample;
    uint64_t end_sample;
    uint8_t sample_size;
};

/* Information about a capture. */
struct CaptureInfo {
    uint64_t file_size;
    uint64_t sample_count;
    uint8_t sample_size;
    uint32_t max_power;
};

/* Holds sample average power by bucket. */
struct PowerBuckets {
    struct Bucket {
        uint32_t power = 0;
        uint8_t count = 0;
    };

    Bucket* p = nullptr;
    size_t size = 0;

    /* Add the power to the bucket at index. */
    void add(size_t index, uint32_t power) {
        if (index < size) {
            auto& b = p[index];
            auto avg = static_cast<uint64_t>(b.power) * b.count;

            b.count++;
            b.power = (power + avg) / b.count;
        }
    }
};

/* Collects capture file metadata and samples power buckets. */
Optional<CaptureInfo> profile_capture(
    const std::filesystem::path& path,
    uint16_t profile_samples = 2'400,
    PowerBuckets* buckets = nullptr);

/* Trims the capture file with the specified range. */
bool trim_capture_with_range(const std::filesystem::path& path, TrimRange range);

/* Trims the capture file with the specified power cutoff. */
bool trim_capture_with_cutoff(
    const std::filesystem::path& path,
    uint32_t cutoff,
    const std::function<void(uint8_t)>& on_progress);

}  // namespace iq

#endif /*__IQ_TRIM_H__*/
