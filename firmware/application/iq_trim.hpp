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

struct TrimRange {
    uint64_t start;
    uint64_t end;
    uint64_t sample_count;
    uint64_t file_size;
    uint32_t max_value;
    uint8_t sample_size;
};

struct PowerBuckets {
    struct Bucket {
        uint8_t power;
    };

    Bucket* p = nullptr;
    size_t size = 0;

    void add(size_t index) {
        // This originally was meant to be an average power for the bucket,
        // but it was a lot of extra math just for a little bit of UI and
        // the math really slowed down processing. Instead, just count the
        // number of samples above the threshold.
        if (index < size) {
            if (p[index].power < 255)
                p[index].power++;
        }
    }
};

inline bool TrimFile(const std::filesystem::path& path, TrimRange range) {
    // NB: range.end should be included in the trimmed result, so '+ sample_size'.
    auto result = trim_file(path, range.start, (range.end - range.start) + range.sample_size);
    return result.ok();
}

template <typename T>
class IQTrimmer {
    static constexpr uint8_t max_amp = 0xFF;
    static constexpr typename T::value_type max_value = std::numeric_limits<typename T::value_type>::max();
    static constexpr uint32_t max_mag_squared{2 * (max_value * max_value)};  // NB: Braces to detect narrowing.

   public:
    static Optional<TrimRange> ComputeTrimRange(
        const std::filesystem::path& path,
        uint32_t amp_threshold,
        PowerBuckets* buckets,
        std::function<void(uint8_t)> on_progress) {
        TrimRange range{};

        File f;
        auto error = f.open(path);
        if (error)
            return {};

        constexpr size_t buffer_size = std::filesystem::max_file_block_size;
        uint8_t buffer[buffer_size];

        bool has_start = false;
        size_t sample_index = 0;
        File::Offset offset = 0;
        uint8_t last_progress = 0;
        size_t samples_per_bucket = 1;
        // Scale from 0-255 to 0-max_mag_squared.
        uint32_t threshold = (static_cast<uint64_t>(max_mag_squared) * amp_threshold) / max_amp;
        T value{};

        range.file_size = f.size();
        range.sample_size = sizeof(T);
        range.sample_count = range.file_size / range.sample_size;

        if (buckets)
            samples_per_bucket = std::max(1ULL, range.sample_count / buckets->size);

        while (true) {
            auto result = f.read(buffer, buffer_size);

            if (!result)
                return {};

            for (size_t i = 0; i < *result; i += sizeof(T)) {
                ++sample_index;

                value = *reinterpret_cast<T*>(&buffer[i]);
                auto real = value.real();
                auto imag = value.imag();
                uint32_t mag_squared = (real * real) + (imag * imag);

                if (mag_squared > range.max_value)
                    range.max_value = mag_squared;

                // Update range if above threshold.
                if (mag_squared >= threshold) {
                    if (has_start) {
                        range.end = offset + i;
                    } else {
                        range.start = offset + i;
                        range.end = range.start;
                        has_start = true;
                    }

                    // Update the optional power bucket.
                    if (buckets) {
                        auto bucket_index = sample_index / samples_per_bucket;
                        buckets->add(bucket_index);
                    }
                }
            }

            if (*result < buffer_size)
                break;

            offset += *result;

            if (on_progress) {
                uint8_t current_progress = 100 * offset / range.file_size;
                if (last_progress != current_progress) {
                    on_progress(current_progress);
                    last_progress = current_progress;
                }
            }
        }

        return range;
    }
};

inline Optional<TrimRange> ComputeTrimRange(
    const std::filesystem::path& path,
    uint32_t amp_threshold,
    PowerBuckets* buckets = nullptr,
    std::function<void(uint8_t)> on_progress = nullptr) {
    Optional<TrimRange> range;
    auto sample_size = std::filesystem::capture_file_sample_size(path);

    switch (sample_size) {
        case sizeof(complex16_t):
            return IQTrimmer<complex16_t>::ComputeTrimRange(path, amp_threshold, buckets, on_progress);

        case sizeof(complex8_t):
            return IQTrimmer<complex8_t>::ComputeTrimRange(path, amp_threshold, buckets, on_progress);

        default:
            return {};
    };
}

#endif /*__IQ_TRIM_H__*/
