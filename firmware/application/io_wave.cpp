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

constexpr size_t wav_header_bytes = 512;  // how much of a file is searched for its chunks

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

    auto header = std::make_unique<uint8_t[]>(wav_header_bytes);
    const auto result = file.read(header.get(), wav_header_bytes);
    wav::Info info;
    if (result.is_error() || !wav::parse_start(header.get(), result.value(), file.size(), info))
        return false;

    // The data chunk may lie beyond what was read (big tag chunks in front): go on
    // from chunk to chunk in the file.
    for (int chunks = 0; !info.data_start && info.next_chunk && chunks < 64; chunks++) {
        uint8_t chunk[8];
        const uint32_t at = info.next_chunk;
        if (file.seek(at).is_error()) return false;
        const auto chunk_result = file.read(chunk, sizeof(chunk));
        if (chunk_result.is_error() || chunk_result.value() < sizeof(chunk)) return false;
        wav::take_chunk(chunk, sizeof(chunk), at, file.size(), info);
    }
    if (!wav::parse_finish(info))
        return false;

    file_ = std::move(file);
    info_ = info;
    title_string = "";
    block_.reset();
    block_number_ = UINT32_MAX;
    if (is_adpcm()) {
        bytes_per_sample = sizeof(int16_t);
        block_ = std::make_unique<uint8_t[]>(info_.block_align + 3);  // + room to line it up, see read_at()
    } else {
        bytes_per_sample = info_.bits / 8;
    }

    find_title(header.get(), result.value());
    rewind();

    last_path = path;
    return true;
}

// Looks for the INAM (title) tag, in a LIST chunk before the data or right after it.
void WAVFileReader::find_title(const uint8_t* header, size_t size) {
    const auto extract = [this](const uint8_t* p, size_t length) {
        for (size_t i = 0; i + 8 <= length; i++) {
            if (memcmp(&p[i], "INAM", 4) != 0) continue;
            size_t title_size = p[i + 4] | (p[i + 5] << 8);
            title_size = std::min<size_t>({title_size, 32, length - i - 8});
            title_string.assign(reinterpret_cast<const char*>(&p[i + 8]), title_size);
            title_string = title_string.c_str();  // cut at the first terminator
            return true;
        }
        return false;
    };

    if (extract(header, std::min<size_t>(size, info_.data_start)))
        return;

    const uint32_t data_end = info_.data_start + info_.data_size;
    if (data_end >= info_.riff_size)
        return;

    uint8_t tail[256];
    file_.seek(data_end);
    const auto result = file_.read(tail, sizeof(tail));
    if (result.is_ok())
        extract(tail, result.value());
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
    const uint32_t samples = std::min<uint64_t>(bytes / bytes_per_sample, sample_count() - position_);

    if (is_adpcm()) {
        const auto result = read_adpcm(static_cast<int16_t*>(buffer), samples / info_.channels);
        if (result.is_ok()) {
            position_ += result.value() / bytes_per_sample;
            bytes_read_ += result.value();
        }
        return result;
    }

    const auto result = read_at(info_.data_start + position_ * bytes_per_sample, buffer, samples * bytes_per_sample);
    if (result.is_ok()) {
        position_ += result.value() / bytes_per_sample;
        bytes_read_ += result.value();
    }
    return result;
}

// ADPCM only decodes forwards. To go back inside a block (reverse playback, or re-reading
// the last frames) the decoder resumes from a checkpoint taken on the way through it,
// instead of decoding the block again from its start.
// Runs from RAM: code in the SPI flash is too slow to decode at more than normal speed.
LOCATE_IN_RAM __attribute__((noinline)) File::Result<File::Size> WAVFileReader::read_adpcm(int16_t* out, uint32_t frames) {
    uint32_t frame = position_ / info_.channels;
    File::Size total = 0;

    while (frames) {
        const uint32_t number = frame / info_.block_frames;
        const uint32_t offset = frame % info_.block_frames;
        if (number != block_number_) {
            // Placed in the buffer so that it lines up with its position in the file.
            const uint32_t position = info_.data_start + number * info_.block_align;
            uint8_t* const block = block_.get() + (position & 3);
            block_number_ = UINT32_MAX;  // until the new block is really there
            const auto seek_result = file_.seek(position);
            if (seek_result.is_error()) return seek_result.error();
            const auto result = read_at(position, block, info_.block_align);
            if (result.is_error()) return result;
            // A short read (truncated file) decodes as a held sample, not as old data.
            if (result.value() < info_.block_align)
                memset(block + result.value(), 0, info_.block_align - result.value());
            block_number_ = number;
            ima_.start(block, info_.channels);
            checkpoints_ = 0;
        } else if (offset < ima_.position()) {
            ima_ = checkpoint_[offset / checkpoint_frames];
        }

        uint32_t n = std::min<uint32_t>(frames, info_.block_frames - offset);
        total += n * info_.channels * sizeof(int16_t);
        frame += n;
        frames -= n;
        for (n += offset - ima_.position(); n > 0; n--) {
            const uint32_t at = ima_.position();
            if (at % checkpoint_frames == 0 && at / checkpoint_frames == checkpoints_)
                checkpoint_[checkpoints_++] = ima_;
            // One call, so the decoder is only inlined once into this RAM resident function.
            const bool wanted = (at >= offset);
            ima_.next(wanted ? out : nullptr);
            if (wanted) out += info_.channels;
        }
    }
    return total;
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
