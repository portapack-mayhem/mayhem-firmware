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

#include "iq_trim.hpp"

#include <memory>

namespace fs = std::filesystem;

namespace iq {

/* Trimming helpers based on the sample type (complex8 or complex16). */
template <typename T>
uint32_t power(T value) {
    auto real = value.real();
    auto imag = value.imag();
    return (real * real) + (imag * imag);
}

/* Collects capture file metadata and samples power buckets. */
template <typename T>
Optional<CaptureInfo> profile_capture(
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
template <typename T>
bool trim_capture(
    const std::filesystem::path& file_path,
    uint32_t cutoff,
    const std::function<void(uint8_t)>& on_progress) {
    constexpr size_t buffer_size = std::filesystem::max_file_block_size;
    uint8_t buffer[buffer_size];
    auto temp_path = file_path + u"-tmp";

    // These need to be heap allocated to avoid overflowing the stack.
    auto src = std::make_unique<File>();
    auto dst = std::make_unique<File>();

    auto error = src->open(file_path);
    if (error) return false;

    error = dst->create(temp_path);
    if (error) return false;

    // For progress reporting.
    uint32_t next_update = 0;
    uint32_t update_interval = src->size() / 25;

    File::Offset offset = 0;
    uint32_t start_offset = 0;
    bool found_start = false;
    T value{};

    // Find first sample above cutoff.
    while (!found_start) {
        auto result = src->read(buffer, buffer_size);
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
            on_progress(100 * offset / src->size());
        }
    }

    uint32_t end_offset = start_offset;
    offset = start_offset;
    src->seek(offset);

    // Write out the rest of the file and look for last good sample.
    while (true) {
        auto result = src->read(buffer, buffer_size);
        if (result.is_error()) return false;

        result = dst->write(buffer, *result);
        if (result.is_error()) return false;

        // Look though all the samples to find the last one above cutoff.
        for (size_t i = 0; i < *result; i += sizeof(T)) {
            auto value = *reinterpret_cast<T*>(&buffer[i]);

            // Still above cutoff?
            if (power(value) >= cutoff)
                end_offset = offset + i;
        }

        if (*result != buffer_size)
            break;  // All done reading.

        offset += *result;

        // Update progress.
        if (on_progress && offset >= next_update) {
            next_update += update_interval;
            on_progress(100 * offset / src->size());
        }
    }

    // Now trim off the end of the output file.
    // NB: end_offset should be included in trimmed file, so add sizeof(T).
    auto length = (end_offset - start_offset) + sizeof(T);
    dst->seek(length);
    dst->truncate();

    // Close the files before renaming/deleting.
    src.reset();
    dst.reset();

    // Delete original and overwrite with temp file.
    delete_file(file_path);
    return rename_file(temp_path, file_path).ok();
}

Optional<CaptureInfo> profile_capture(
    const fs::path& path,
    uint16_t profile_samples,
    PowerBuckets* buckets) {
    auto sample_size = fs::capture_file_sample_size(path);

    switch (sample_size) {
        case sizeof(complex16_t):
            return profile_capture<complex16_t>(path, profile_samples, buckets);

        case sizeof(complex8_t):
            return profile_capture<complex8_t>(path, profile_samples, buckets);

        default:
            return {};
    };
}

bool trim_capture_with_range(const fs::path& path, TrimRange range) {
    auto start_byte = range.start_sample * range.sample_size;
    auto end_byte = (range.end_sample + 1) * range.sample_size;
    auto result = trim_file(path, start_byte, end_byte - start_byte);
    return result.ok();
}

bool trim_capture_with_cutoff(
    const fs::path& path,
    uint32_t cutoff,
    const std::function<void(uint8_t)>& on_progress) {
    auto sample_size = fs::capture_file_sample_size(path);
    switch (sample_size) {
        case sizeof(complex16_t):
            return trim_capture<complex16_t>(path, cutoff, on_progress);

        case sizeof(complex8_t):
            return trim_capture<complex8_t>(path, cutoff, on_progress);

        default:
            return false;
    };
}

}  // namespace iq