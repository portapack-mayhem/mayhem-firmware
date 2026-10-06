/*
 * Copyright (C) 2026 SecLBL
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

#ifndef __MUSIC_WAV_SOURCE_H__
#define __MUSIC_WAV_SOURCE_H__

/* WAV header parsing and IMA ADPCM decoding for the Music app. No firmware dependencies,
 * so test/application/test_music_wav_source.cpp can check it on a PC. */

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace ui::external_app::music {

struct WavInfo {
    enum : uint16_t { PCM = 0x01,
                      IMA_ADPCM = 0x11 };
    static constexpr uint16_t max_block_align = 2048;

    uint16_t format{0};
    uint16_t channels{0};
    uint32_t rate{0};
    uint16_t block_align{0};   // bytes: one frame (PCM) or one compressed block (ADPCM)
    uint16_t block_frames{0};  // frames in an ADPCM block
    uint32_t data_start{0};
    uint32_t frames{0};
};

/* Walks the chunks at the start of a WAV file. Returns nullptr, or what is wrong with it.
 * NB: the "data" chunk has to start within `size` bytes; read more of the file if
 * WAVs with big tag chunks in front (cover art) ever need to play. */
inline const char* parse_wav(const uint8_t* p, size_t size, uint32_t file_size, WavInfo& info) {
    const auto u16 = [p](size_t at) { return (uint16_t)(p[at] | (p[at + 1] << 8)); };
    const auto u32 = [p](size_t at) { return (uint32_t)(p[at] | (p[at + 1] << 8) | (p[at + 2] << 16) | ((uint32_t)p[at + 3] << 24)); };

    info = WavInfo{};
    if (size < 12 || memcmp(p, "RIFF", 4) != 0 || memcmp(p + 8, "WAVE", 4) != 0)
        return "NOT A WAV FILE";

    uint16_t bits = 0;
    uint32_t data_size = 0;
    for (size_t at = 12; at + 8 <= size;) {
        const uint32_t length = u32(at + 4);
        if (memcmp(p + at, "fmt ", 4) == 0 && at + 24 <= size) {
            info.format = u16(at + 8);
            info.channels = u16(at + 10);
            info.rate = u32(at + 12);
            info.block_align = u16(at + 20);
            bits = u16(at + 22);
        } else if (memcmp(p + at, "data", 4) == 0) {
            info.data_start = at + 8;
            data_size = (file_size - info.data_start < length) ? file_size - info.data_start : length;
            break;
        }
        if (length > size - at - 8) break;  // the next chunk starts outside what was read
        at += 8 + length + (length & 1);
    }

    if (!info.data_start || info.channels < 1 || info.channels > 2 || info.rate != 48000)
        return "NEED 48K WAV";

    if (info.format == WavInfo::PCM && bits == 16) {
        info.block_align = info.channels * sizeof(int16_t);
        info.frames = data_size / info.block_align;
    } else if (info.format == WavInfo::IMA_ADPCM && bits == 4 &&
               info.block_align > 4 * info.channels && info.block_align <= WavInfo::max_block_align &&
               (info.block_align - 4 * info.channels) % (4 * info.channels) == 0) {
        // Per channel a 4 byte header holding the first sample, then 4 bit codes in whole
        // groups of 4 bytes per channel.
        info.block_frames = 1 + (info.block_align - 4 * info.channels) * 2 / info.channels;
        // NB: a short last block is dropped (under 50 ms of the track's end).
        info.frames = data_size / info.block_align * info.block_frames;
    } else {
        return "NEED PCM16 OR ADPCM";
    }
    return nullptr;
}

/* Decodes one IMA ADPCM block of a WAV file, frame by frame, front to back. */
class ImaBlock {
   public:
    void start(const uint8_t* block, uint16_t channels) {
        block_ = block;
        channels_ = channels;
        position_ = 0;
        for (uint16_t c = 0; c < channels; c++) {
            predictor_[c] = (int16_t)(block[4 * c] | (block[4 * c + 1] << 8));
            index_[c] = (block[4 * c + 2] > 88) ? 88 : block[4 * c + 2];
        }
    }

    // Number of the frame next() produces.
    uint16_t position() const { return position_; }

    // Decodes the next frame; `out` may be null to skip over it.
    void next(int16_t* out) {
        static const int8_t index_step[8] = {-1, -1, -1, -1, 2, 4, 6, 8};
        static const uint16_t step_size[89] = {
            7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
            50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230,
            253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
            1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
            3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
            11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
            32767};

        // Frame 0 is the header sample. After it the codes come 8 per channel at a time
        // (4 bytes of the left channel, 4 of the right, ...), low nibble first.
        if (position_ > 0) {
            const uint32_t code_number = position_ - 1;
            const uint8_t* group = block_ + 4 * channels_ + (code_number / 8) * 4 * channels_;
            for (uint16_t c = 0; c < channels_; c++) {
                const uint8_t byte = group[4 * c + (code_number % 8) / 2];
                const uint8_t code = (code_number & 1) ? (byte >> 4) : (byte & 0x0F);

                const int32_t step = step_size[index_[c]];
                int32_t delta = step >> 3;
                if (code & 4) delta += step;
                if (code & 2) delta += step >> 1;
                if (code & 1) delta += step >> 2;
                int32_t value = predictor_[c] + ((code & 8) ? -delta : delta);
                if (value > 32767) value = 32767;
                if (value < -32768) value = -32768;
                predictor_[c] = value;

                int32_t index = index_[c] + index_step[code & 7];
                index_[c] = (index < 0) ? 0 : (index > 88) ? 88
                                                           : index;
            }
        }
        if (out)
            for (uint16_t c = 0; c < channels_; c++)
                out[c] = predictor_[c];
        position_++;
    }

   private:
    const uint8_t* block_{nullptr};
    uint16_t channels_{0};
    uint16_t position_{0};
    int16_t predictor_[2]{};
    uint8_t index_[2]{};
};

}  // namespace ui::external_app::music

#endif /*__MUSIC_WAV_SOURCE_H__*/
