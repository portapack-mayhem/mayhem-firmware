/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#ifndef __IO_WAVE_H
#define __IO_WAVE_H

#include "io_file.hpp"

#include "file.hpp"
#include "optional.hpp"
#include "wav_format.hpp"

#include <string.h>
#include <climits>
#include <memory>

struct fmt_pcm_t {
    constexpr fmt_pcm_t(
        const uint32_t sampling_rate)
        : nSamplesPerSec{sampling_rate},
          nAvgBytesPerSec{nSamplesPerSec * 2}  // nBlockAlign = 2
    {
    }

   private:
    uint8_t ckID[4]{'f', 'm', 't', ' '};
    uint32_t cksize{16};
    uint16_t wFormatTag{0x0001};
    uint16_t nChannels{1};
    uint32_t nSamplesPerSec;
    uint32_t nAvgBytesPerSec;
    uint16_t nBlockAlign{2};
    uint16_t wBitsPerSample{16};
};

struct data_t {
    constexpr data_t(
        const uint32_t size)
        : cksize{size} {
    }

   private:
    uint8_t ckID[4]{'d', 'a', 't', 'a'};
    uint32_t cksize{0};
};

struct header_t {
    constexpr header_t(
        const uint32_t sampling_rate,
        const uint32_t data_chunk_size,
        const uint32_t info_chunk_size)
        : cksize{sizeof(header_t) + data_chunk_size + info_chunk_size - 8},
          fmt{sampling_rate},
          data{data_chunk_size} {
    }

   private:
    uint8_t riff_id[4]{'R', 'I', 'F', 'F'};
    uint32_t cksize{0};
    uint8_t wave_id[4]{'W', 'A', 'V', 'E'};
    fmt_pcm_t fmt;
    data_t data;
};

struct tags_t {
    constexpr tags_t(
        const std::string& title_str) {
        strncpy(title, title_str.c_str(), sizeof(title) - 1);
        title[sizeof(title) - 1] = '\0';
        cksize = sizeof(tags_t) - 8;
    }

   private:
    uint8_t list_id[4]{'L', 'I', 'S', 'T'};
    uint32_t cksize{0};
    uint8_t info_id[4]{'I', 'N', 'F', 'O'};
    uint8_t iart_id[4]{'I', 'A', 'R', 'T'};
    uint32_t sckiart_size{12};
    char artist[12]{"PortaPack\0\0"};
    uint8_t inam_id[4]{'I', 'N', 'A', 'M'};
    uint32_t sckinam_size{64};
    char title[64]{0};
};

/* Reads PCM (8 or 16 bit) and IMA ADPCM WAV files. ADPCM is decoded on the fly: read(),
 * data_seek() and the size getters then behave as for a 16 bit PCM file. */
class WAVFileReader : public FileReader {
   public:
    WAVFileReader() = default;

    WAVFileReader(const WAVFileReader&) = delete;
    WAVFileReader& operator=(const WAVFileReader&) = delete;
    WAVFileReader(WAVFileReader&&) = delete;
    WAVFileReader& operator=(WAVFileReader&&) = delete;

    virtual ~WAVFileReader() = default;

    bool open(const std::filesystem::path& path);
    File::Result<File::Size> read(void* const buffer, const File::Size bytes) override;
    void data_seek(const uint64_t Offset);  // in samples, counting every channel
    void rewind();
    uint32_t ms_duration();
    uint16_t channels();
    uint32_t sample_rate();
    uint32_t data_size();
    uint32_t sample_count();  // counting every channel
    uint32_t frame_count();   // one frame is one sample for every channel
    uint16_t bits_per_sample();
    bool is_adpcm();
    std::string title();

   private:
    wav::Info info_{};
    uint32_t bytes_per_sample{};  // of what read() returns
    uint32_t position_{0};        // next sample read() returns, counting every channel
    Optional<File::Error> seek_error_{};
    std::string title_string{};
    std::filesystem::path last_path{};

    // ADPCM: the compressed block being decoded, which one it is, and the decoder.
    std::unique_ptr<uint8_t[]> block_{};
    uint32_t block_number_{UINT32_MAX};
    wav::ImaBlock ima_{};

    // The decoder as it stood every checkpoint_frames frames into the current block.
    static constexpr uint32_t checkpoint_frames = 512;
    wav::ImaBlock checkpoint_[wav::Info::max_adpcm_frames / checkpoint_frames + 1]{};
    uint32_t checkpoints_{0};

    File::Result<File::Size> read_at(uint32_t position, void* out, uint32_t bytes);
    File::Result<File::Size> read_adpcm(int16_t* out, uint32_t frames);
    void find_title(const uint8_t* header, size_t size);
};

class WAVFileWriter : public FileWriter {
   public:
    WAVFileWriter() = default;

    WAVFileWriter(const WAVFileWriter&) = delete;
    WAVFileWriter& operator=(const WAVFileWriter&) = delete;
    WAVFileWriter(WAVFileWriter&&) = delete;
    WAVFileWriter& operator=(WAVFileWriter&&) = delete;

    ~WAVFileWriter() {
        write_tags();
        update_header();
    }

    Optional<File::Error> create(
        const std::filesystem::path& filename,
        size_t sampling_rate,
        const std::string& title_set);

   private:
    uint32_t sampling_rate{0};
    uint32_t info_chunk_size{0};
    std::string title{};

    Optional<File::Error> update_header();
    Optional<File::Error> write_tags();
};

#endif
