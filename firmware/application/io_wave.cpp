/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2024 Mark Thompson
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

#include "io_wave.hpp"
#include "utility.hpp"

#include <algorithm>

bool WAVFileReader::open(const std::filesystem::path& path) {
    // Already open ?
    if (path.string() == last_path.string()) {
        rewind();
        return true;
    }

    // Look at the new file on the side: if it turns out not to be usable, the reader
    // stays on the file it had, for callers that carry on with it.
    File file;
    if (file.open(path).is_valid())
        return false;

    // Walk from chunk to chunk, wherever in the file they are. Each one is read with
    // enough of its content for the fields of "fmt ".
    uint8_t chunk[8 + 34];
    wav::Info info;
    auto result = file.read(chunk, 12);
    if (result.is_error() || result.value() < 12 || !wav::parse_riff(chunk, info))
        return false;
    for (int chunks = 0; !info.data_start && info.next_chunk && chunks < 64; chunks++) {
        const uint32_t at = info.next_chunk;
        if (file.seek(at).is_error()) return false;
        result = file.read(chunk, sizeof(chunk));
        if (result.is_error() || result.value() < 8) return false;
        wav::take_chunk(chunk, result.value(), at, file.size(), info);
    }
    if (!wav::parse_finish(info))
        return false;

    file_ = std::move(file);
    info_ = info;
    block_.reset();
    bytes_per_sample = is_adpcm() ? sizeof(int16_t) : info_.bits / 8;

    find_title();
    rewind();

    last_path = path;
    return true;
}

// Looks for the INAM (title) tag: in the chunks before the data in most files, right
// after the data in the ones this firmware records.
void WAVFileReader::find_title() {
    title_string = "";

    uint8_t window[256];
    const uint32_t data_end = info_.data_start + info_.data_size;
    const uint32_t from[2] = {12, data_end};
    const uint32_t to[2] = {info_.data_start, info_.riff_size};
    for (int part = 0; part < 2; part++) {
        if (from[part] >= to[part]) continue;
        file_.seek(from[part]);
        const auto result = file_.read(window, std::min<uint32_t>(sizeof(window), to[part] - from[part]));
        if (result.is_error()) return;

        for (size_t i = 0; i + 8 <= result.value(); i++) {
            if (memcmp(&window[i], "INAM", 4) != 0) continue;
            // The length field, capped at 32 characters and at the first terminator.
            const char* const text = reinterpret_cast<const char*>(&window[i + 8]);
            const size_t limit = std::min<size_t>(std::min<size_t>(window[i + 4], 32), result.value() - i - 8);
            size_t length = 0;
            while (length < limit && text[length]) length++;
            title_string.assign(text, length);
            return;
        }
    }
}

void WAVFileReader::rewind() {
    data_seek(0);
}

std::string WAVFileReader::title() {
    return title_string;
}

uint32_t WAVFileReader::ms_duration() {
    return info_.rate ? (uint64_t)info_.frames * 1000 / info_.rate : 0;
}

void WAVFileReader::data_seek(const uint64_t Offset) {
    position_ = std::min<uint64_t>(Offset, sample_count());
    seek_error_ = Optional<File::Error>{};
    if (!is_adpcm()) {
        const auto result = file_.seek(info_.data_start + position_ * bytes_per_sample);
        if (result.is_error()) seek_error_ = result.error();
    }
}

// The SD card driver corrupts whole sectors read into a buffer that is not word aligned
// relative to the file position, and for whole sectors FatFs hands it the caller's buffer.
// Such reads are split so that they go through FatFs's own aligned sector buffer instead.
File::Result<File::Size> WAVFileReader::read_at(uint32_t position, void* out, uint32_t bytes) {
    constexpr uint32_t sector = 512;
    auto p = static_cast<uint8_t*>(out);

    if (((reinterpret_cast<uintptr_t>(p) - position) & 3) == 0)
        return file_.read(p, bytes);

    File::Size total = 0;
    while (bytes) {
        uint32_t n = std::min<uint32_t>(bytes, sector - position % sector);
        if (n == sector) n = sector / 2;
        const auto result = file_.read(p, n);
        if (result.is_error()) return result;
        total += result.value();
        if (result.value() < n) break;
        p += n;
        position += n;
        bytes -= n;
    }
    return total;
}

File::Result<File::Size> WAVFileReader::read(void* const buffer, const File::Size bytes) {
    if (seek_error_.is_valid()) return seek_error_.value();
    if (!bytes_per_sample) return File::Size{0};

    // Never past the end of the audio: other chunks may follow it in the file.
    const uint32_t wanted = std::min<File::Size>(bytes, UINT32_MAX);
    const uint32_t samples = std::min<uint32_t>(wanted / bytes_per_sample, sample_count() - position_);

    const auto result = is_adpcm()
                            ? read_adpcm(static_cast<int16_t*>(buffer), samples / info_.channels)
                            : read_at(info_.data_start + position_ * bytes_per_sample, buffer, samples * bytes_per_sample);
    if (result.is_ok()) {
        position_ += result.value() / bytes_per_sample;
        bytes_read_ += result.value();
    }
    return result;
}

File::Result<File::Size> WAVFileReader::read_adpcm(int16_t* out, uint32_t frames) {
    if (!block_) {
        block_ = std::make_unique<uint8_t[]>(wav::AdpcmFrames::buffer_bytes(info_));
        adpcm_.start(info_, block_.get());
    }

    Optional<File::Error> error{};
    const bool ok = adpcm_.read(position_ / info_.channels, frames, out,
                                [this, &error](uint32_t position, uint8_t* dest, uint32_t bytes) -> int32_t {
                                    auto result = file_.seek(position);
                                    if (result.is_ok()) result = read_at(position, dest, bytes);
                                    if (result.is_ok()) return result.value();
                                    error = result.error();
                                    return -1;
                                });
    if (!ok) return error.value();
    return File::Size{frames * info_.channels * sizeof(int16_t)};
}

uint16_t WAVFileReader::channels() {
    return info_.channels;
}

uint32_t WAVFileReader::sample_rate() {
    return info_.rate;
}

uint32_t WAVFileReader::data_size() {
    return sample_count() * bytes_per_sample;
}

uint32_t WAVFileReader::sample_count() {
    return info_.frames * info_.channels;
}

uint32_t WAVFileReader::frame_count() {
    return info_.frames;
}

uint16_t WAVFileReader::bits_per_sample() {
    return bytes_per_sample * 8;
}

bool WAVFileReader::is_adpcm() {
    return info_.format == wav::Info::IMA_ADPCM;
}

Optional<File::Error> WAVFileWriter::create(
    const std::filesystem::path& filename,
    size_t sampling_rate_set,
    const std::string& title_set) {
    sampling_rate = sampling_rate_set;
    title = title_set;
    const auto create_error = FileWriter::create(filename);
    if (create_error.is_valid()) {
        return create_error;
    } else {
        return update_header();
    }
}

Optional<File::Error> WAVFileWriter::update_header() {
    header_t header{sampling_rate, (uint32_t)bytes_written_ - sizeof(header_t), info_chunk_size};

    const auto seek_0_result = file_.seek(0);
    if (seek_0_result.is_error()) {
        return seek_0_result.error();
    }

    const auto old_position = seek_0_result.value();

    const auto write_result = file_.write(&header, sizeof(header));
    if (write_result.is_error()) {
        return write_result.error();
    }

    const auto seek_old_result = file_.seek(old_position);
    if (seek_old_result.is_error()) {
        return seek_old_result.error();
    }

    return {};
}

Optional<File::Error> WAVFileWriter::write_tags() {
    tags_t tags{title};

    const auto write_result = file_.write(&tags, sizeof(tags));
    if (write_result.is_error()) {
        return write_result.error();
    }

    info_chunk_size = sizeof(tags);

    return {};
}
