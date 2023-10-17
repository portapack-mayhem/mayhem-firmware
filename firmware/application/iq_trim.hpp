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

/* Information about a capture. */
struct CaptureInfo {
    uint64_t file_size;
    uint64_t sample_count;
    uint8_t sample_size;
    uint32_t max_power;
    uint32_t max_iq;
};

/* Holds sample average power by bucket. */
struct PowerBuckets {
    struct Bucket {
        uint32_t power = 0;
        uint8_t count = 0;
    };

    Bucket* p = nullptr;
    const size_t size = 0;

    /* Add the power to the bucket average at index. */
    void add(size_t index, uint32_t power) {
        if (index < size) {
            auto& b = p[index];
            auto avg = static_cast<uint64_t>(b.power) * b.count;

            b.count++;
            b.power = (power + avg) / b.count;
        }
    }
};

/* Data needed to trim a capture by sample range.
 * end_sample is the sample *after* the last to keep. */
struct TrimRange {
    uint64_t start_sample;
    uint64_t end_sample;
    uint8_t sample_size;
};

/* Collects capture file metadata and samples power buckets. */
Optional<CaptureInfo> profile_capture(
    const std::filesystem::path& path,
    PowerBuckets& buckets,
    uint8_t samples_per_bucket = 10);

/* Computes the trimming range given profiling info.
 * Cutoff percent is a number 1-100. */
TrimRange compute_trim_range(
    CaptureInfo info,
    const PowerBuckets& buckets,
    uint8_t cutoff_percent);

/* Multiplies samples in an IQ buffer by amplification value */
void amplify_iq_buffer(
    uint8_t* buffer,
    uint32_t length,
    uint32_t amplification,
    uint8_t sample_size);

/* Trims the capture file with the specified range. */
bool trim_capture_with_range(
    const std::filesystem::path& path,
    TrimRange range,
    const std::function<void(uint8_t)>& on_progress,
    const uint32_t amplification);

}  // namespace iq

#endif /*__IQ_TRIM_H__*/
