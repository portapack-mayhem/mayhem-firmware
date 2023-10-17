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
#include "string_format.hpp"

namespace fs = std::filesystem;

namespace iq {

/* Trimming helpers based on the sample type (complex8 or complex16). */
template <typename T>
uint32_t power(T value) {
    auto real = value.real();
    auto imag = value.imag();
    return (real * real) + (imag * imag);
}

template <typename T>
uint32_t iq_max(T value) {
    auto real = abs(value.real());
    auto imag = abs(value.imag());
    return (real > imag) ? real : imag;
}

/* Collects capture file metadata and sample power buckets. */
template <typename T>
Optional<CaptureInfo> profile_capture(
    const std::filesystem::path& path,
    PowerBuckets& buckets,
    uint8_t samples_per_bucket) {
    File f;
    auto error = f.open(path);
    if (error)
        return {};

    CaptureInfo info{
        .file_size = f.size(),
        .sample_count = f.size() / sizeof(T),
        .sample_size = sizeof(T),
        .max_power = 0,
        .max_iq = 0};

    auto profile_samples = buckets.size * samples_per_bucket;
    auto sample_interval = info.sample_count / profile_samples;
    uint32_t bucket_width = std::max(1ULL, info.sample_count / buckets.size);
    uint64_t sample_index = 0;
    T value{};

    while (true) {
        f.seek(sample_index * info.sample_size);
        auto result = f.read(&value, info.sample_size);
        if (!result) return {};  // Read failed.

        if (*result != info.sample_size)
            break;  // EOF

        auto max_iq = iq_max(value);
        if (max_iq > info.max_iq)
            info.max_iq = max_iq;

        auto mag_squared = power(value);
        if (mag_squared > info.max_power)
            info.max_power = mag_squared;

        auto bucket_index = sample_index / bucket_width;
        buckets.add(bucket_index, mag_squared);
        sample_index += sample_interval;
    }

    return info;
}

Optional<CaptureInfo> profile_capture(
    const fs::path& path,
    PowerBuckets& buckets,
    uint8_t samples_per_bucket) {
    auto sample_size = fs::capture_file_sample_size(path);

    switch (sample_size) {
        case sizeof(complex16_t):
            return profile_capture<complex16_t>(path, buckets, samples_per_bucket);

        case sizeof(complex8_t):
            return profile_capture<complex8_t>(path, buckets, samples_per_bucket);

        default:
            return {};
    };
}

TrimRange compute_trim_range(
    CaptureInfo info,
    const PowerBuckets& buckets,
    uint8_t cutoff_percent) {
    bool has_start = false;
    uint8_t start_bucket = 0;
    uint8_t end_bucket = 0;

    uint32_t power_cutoff = cutoff_percent * static_cast<uint64_t>(info.max_power) / 100;

    for (size_t i = 0; i < buckets.size; ++i) {
        auto power = buckets.p[i].power;

        if (power > power_cutoff) {
            if (has_start)
                end_bucket = i;
            else {
                start_bucket = i;
                end_bucket = i;
                has_start = true;
            }
        }
    }

    // End should be the first bucket after the last with signal.
    // This makes the math downstream simpler. NB: may be > buckets.size.
    end_bucket++;

    auto samples_per_bucket = info.sample_count / buckets.size;
    return {
        start_bucket * samples_per_bucket,
        end_bucket * samples_per_bucket,
        info.sample_size};
}

void amplify_iq_buffer(uint8_t* buffer, uint32_t length, uint32_t amplification, uint8_t sample_size) {
    uint32_t mult_count = length / sample_size / 2;

    switch (sample_size) {
        case sizeof(complex16_t): {
            int16_t* buf_ptr = (int16_t*)buffer;
            for (uint32_t i = 0; i < mult_count; i++) {
                int32_t val = *buf_ptr * amplification;
                if (val > 0x7FFF)
                    val = 0x7FFF;
                else if (val < -0x7FFF)
                    val = -0x7FFF;
                *buf_ptr++ = val;
            }
            break;
        }

        case sizeof(complex8_t): {
            int8_t* buf_ptr = (int8_t*)buffer;
            for (uint32_t i = 0; i < mult_count; i++) {
                int32_t val = *buf_ptr * amplification;
                if (val > 0x7F)
                    val = 0x7F;
                else if (val < -0x7F)
                    val = -0x7F;
                *buf_ptr++ = val;
            }
            break;
        }

        default:
            break;
    }
}

bool trim_capture_with_range(
    const fs::path& path,
    TrimRange range,
    const std::function<void(uint8_t)>& on_progress,
    const uint32_t amplification) {
    constexpr size_t buffer_size = std::filesystem::max_file_block_size;
    uint8_t buffer[buffer_size];
    auto temp_path = path + u"-tmp";
    auto sample_size = fs::capture_file_sample_size(path);

    // end_sample is the first sample to _not_ include.
    auto start_byte = range.start_sample * range.sample_size;
    auto end_byte = (range.end_sample * range.sample_size);
    auto length = end_byte - start_byte;

    // 'File' is 556 bytes! Heap alloc to avoid overflowing the stack.
    auto src = std::make_unique<File>();
    auto dst = std::make_unique<File>();

    auto error = src->open(path);
    if (error) return false;

    error = dst->create(temp_path);
    if (error) return false;

    src->seek(start_byte);
    auto processed = 0UL;
    auto next_report = 0UL;
    auto report_interval = length / 20UL;

    while (true) {
        auto result = src->read(buffer, buffer_size);
        if (result.is_error()) return false;

        auto remaining = length - processed;
        auto to_write = std::min(remaining, *result);

        if (amplification > 1)
            amplify_iq_buffer(buffer, to_write, amplification, sample_size);

        result = dst->write(buffer, to_write);
        if (result.is_error()) return false;

        processed += *result;

        if (*result < buffer_size || processed >= length)
            break;

        if (processed >= next_report) {
            on_progress(100 * processed / length);
            next_report += report_interval;
        }
    }

    // Close files before renaming/deleting.
    src.reset();
    dst.reset();

    // Delete original and overwrite with temp file.
    delete_file(path);
    rename_file(temp_path, path);
    return true;
}

}  // namespace iq