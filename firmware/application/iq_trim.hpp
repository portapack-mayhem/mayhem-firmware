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
        uint32_t power;
        uint8_t count;
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

/* Trims the capture file with the specified range. */
inline bool TrimCapture(const std::filesystem::path& path, TrimRange range) {
    auto start_byte = range.start_sample * range.sample_size;
    auto end_byte = (range.end_sample + 1) * range.sample_size;
    auto result = trim_file(path, start_byte, end_byte - start_byte);
    return result.ok();
}

/* Trimming helper based on the sample type (complex8 or complex16). */
template <typename T>
class IQTrimmer {
    static constexpr uint8_t max_amp = 0xFF;
    static constexpr typename T::value_type max_value = std::numeric_limits<typename T::value_type>::max();
    static constexpr uint32_t max_mag_squared{2 * (max_value * max_value)};  // NB: Braces to detect narrowing.

   public:
    /* Collects capture file metadata and samples power buckets. */
    static Optional<CaptureInfo> ProfileCapture(
        const std::filesystem::path& path,
        uint16_t profile_samples,
        PowerBuckets* buckets) {
        File f;
        auto error = f.open(path);
        if (error)
            return {};

        CaptureInfo info{
            .file_size = f.size(),
            .sample_count = f.size() / sizeof(T),
            .sample_size = sizeof(T),
            .max_power = 0};

        auto sample_interval = info.sample_count / profile_samples;
        uint32_t samples_per_bucket = 1;
        uint64_t sample_index = 0;
        T value{};

        if (buckets)
            samples_per_bucket = std::max(1ULL, info.sample_count / buckets->size);

        while (true) {
            f.seek(sample_index * info.sample_size);
            auto result = f.read(&value, info.sample_size);
            if (!result) return {};  // Read failed.

            if (*result != info.sample_size)
                break;  // EOF

            auto mag_squared = power(value);

            if (mag_squared > info.max_power)
                info.max_power = mag_squared;

            // Fill in optional power buckets.
            if (buckets) {
                auto bucket_index = sample_index / samples_per_bucket;
                buckets->add(bucket_index, mag_squared);
            }

            sample_index += sample_interval;
        }

        return info;
    }

    /* Trims the capture file with the specified power cutoff.
     * This trims in-place without needing a second pass. */
    static bool TrimCapture(
        const std::filesystem::path& path,
        uint32_t cutoff,
        std::function<void(uint8_t)> on_progress) {
        constexpr size_t buffer_size = std::filesystem::max_file_block_size;
        uint8_t buffer[buffer_size];
        auto temp_path = path + u"-tmp";

        /* Scope for File instances. */
        {
            File src;
            File dst;

            auto error = src.open(path);
            if (error) return false;

            error = dst.create(temp_path);
            if (error) return false;

            // For progress reporting.
            uint32_t update_interval = src.size() / 25;
            uint32_t next_update = 0;

            File::Offset offset = 0;
            uint32_t start_offset = 0;
            bool found_start = false;
            T value{};

            // Find first sample above cutoff.
            while (!found_start) {
                auto result = src.read(buffer, buffer_size);
                if (!result) return false;  // Bad read

                for (size_t i = 0; i < *result; i += sizeof(T)) {
                    auto value = *reinterpret_cast<T*>(&buffer[i]);
                    auto mag_squared = power(value);

                    if (mag_squared >= cutoff) {
                        start_offset = offset + i;
                        found_start = true;
                        break;
                    }
                }

                if (*result != buffer_size && !found_start)
                    return false;  // EOF without finding a sample.

                offset += *result;

                // Update progress.
                if (on_progress && offset >= next_update) {
                    next_update += update_interval;
                    on_progress(100 * offset / src.size());
                }
            }

            uint32_t end_offset = start_offset;
            offset = start_offset;
            src.seek(offset);

            // Write out the rest of the file and look for last good sample.
            while (true) {
                auto result = src.read(buffer, buffer_size);
                if (result.is_error()) return false;

                result = dst.write(buffer, *result);
                if (result.is_error()) return false;

                // Look though all the samples to find the last one above cutoff.
                for (size_t i = 0; i < *result; i += sizeof(T)) {
                    auto value = *reinterpret_cast<T*>(&buffer[i]);
                    auto mag_squared = power(value);

                    // Still above cutoff?
                    if (mag_squared >= cutoff)
                        end_offset = offset + i;
                }

                if (*result != buffer_size)
                    break;  // All done reading.

                offset += *result;

                // Update progress.
                if (on_progress && offset >= next_update) {
                    next_update += update_interval;
                    on_progress(100 * offset / src.size());
                }
            }

            // Now trim off the end of the output file.
            // NB: end_offset should be included in trimmed file, so add sizeof(T).
            auto length = (end_offset - start_offset) + sizeof(T);
            dst.seek(length);
            dst.truncate();
        }

        // Delete original and overwrite with temp file.
        delete_file(path);
        return rename_file(temp_path, path).ok();
    }

   private:
    static uint32_t power(T value) {
        auto real = value.real();
        auto imag = value.imag();
        return (real * real) + (imag * imag);
    }
};

/* Collects capture file metadata and samples power buckets. */
inline Optional<CaptureInfo> ProfileCapture(
    const std::filesystem::path& path,
    uint16_t profile_samples = 2'400,
    PowerBuckets* buckets = nullptr) {
    auto sample_size = std::filesystem::capture_file_sample_size(path);

    switch (sample_size) {
        case sizeof(complex16_t):
            return IQTrimmer<complex16_t>::ProfileCapture(path, profile_samples, buckets);

        case sizeof(complex8_t):
            return IQTrimmer<complex8_t>::ProfileCapture(path, profile_samples, buckets);

        default:
            return {};
    };
}

/* Trims the capture file with the specified power cutoff. */
inline bool TrimCapture(
    const std::filesystem::path& path,
    uint32_t cutoff,
    std::function<void(uint8_t)> on_progress = nullptr) {
    auto sample_size = std::filesystem::capture_file_sample_size(path);

    switch (sample_size) {
        case sizeof(complex16_t):
            return IQTrimmer<complex16_t>::TrimCapture(path, cutoff, on_progress);

        case sizeof(complex8_t):
            return IQTrimmer<complex8_t>::TrimCapture(path, cutoff, on_progress);

        default:
            return false;
    };
}

#endif /*__IQ_TRIM_H__*/
