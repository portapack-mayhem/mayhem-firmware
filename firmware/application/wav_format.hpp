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

#ifndef __WAV_FORMAT_H__
#define __WAV_FORMAT_H__

/* WAV header parsing and IMA ADPCM decoding, used by WAVFileReader (io_wave.cpp). No
 * firmware dependencies, so test/application/test_wav_format.cpp can check it on a PC. */

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace wav {

struct Info {
    enum : uint16_t { PCM = 0x01,
                      EXTENSIBLE = 0xFFFE,
                      IMA_ADPCM = 0x11 };
    static constexpr uint16_t max_adpcm_block = 2048;
    static constexpr uint16_t max_adpcm_frames = 1 + (max_adpcm_block - 4) * 2;  // in a mono block

    uint16_t format{0};
    uint16_t channels{0};
    uint32_t rate{0};
    uint16_t bits{0};          // per sample in the file: 8 or 16 for PCM, 4 for ADPCM
    uint16_t block_align{0};   // bytes: one frame (PCM) or one compressed block (ADPCM)
    uint16_t block_frames{0};  // frames in an ADPCM block
    uint32_t riff_size{0};     // of the whole RIFF chunk, header included
    uint32_t next_chunk{0};    // where the chunk walk has to go on, 0 once it is over
    uint32_t data_start{0};
    uint32_t data_size{0};    // bytes of the data chunk that are really in the file
    uint32_t fact_frames{0};  // frame count from the "fact" chunk, 0 if there is none
    uint32_t frames{0};       // one frame is one sample for every channel
};

// Chunk tags as the little endian numbers they read as: comparing those is smaller than
// calling memcmp.
constexpr uint32_t tag_data = 0x61746164;
constexpr uint32_t tag_fmt = 0x20746D66;
constexpr uint32_t tag_fact = 0x74636166;

/* Takes one chunk whose 8 byte header `p` sits at `at` in the file, the rest of `size`
 * bytes following it. Sets data_start for the data chunk; otherwise leaves in next_chunk
 * where the following chunk starts, or 0 if there is none. */
inline void take_chunk(const uint8_t* p, size_t size, uint32_t at, uint32_t file_size, Info& info) {
    const auto u16 = [p](size_t i) { return (uint16_t)(p[i] | (p[i + 1] << 8)); };
    const auto u32 = [p](size_t i) { return (uint32_t)(p[i] | (p[i + 1] << 8) | (p[i + 2] << 16) | ((uint32_t)p[i + 3] << 24)); };

    const uint32_t length = u32(4);
    info.next_chunk = 0;
    const uint32_t tag = u32(0);
    if (tag == tag_data) {
        info.data_start = at + 8;
        info.data_size = (file_size - info.data_start < length) ? file_size - info.data_start : length;
        return;
    }
    if (tag == tag_fact && size >= 12) info.fact_frames = u32(8);
    if (tag == tag_fmt && size >= 24) {
        info.format = u16(8);
        info.channels = u16(10);
        info.rate = u32(12);
        info.block_align = u16(20);
        info.bits = u16(22);
        // WAVE_FORMAT_EXTENSIBLE: the real format leads the sub format GUID.
        if (info.format == Info::EXTENSIBLE && length >= 40 && size >= 34)
            info.format = u16(32);
    }
    // A chunk that claims to reach past the end of the file ends the walk.
    if ((uint64_t)length + (length & 1) + 8 <= file_size - at - 8)
        info.next_chunk = at + 8 + length + (length & 1);
}

/* Takes the 12 bytes a WAV file starts with. Returns false if it is not one; otherwise
 * next_chunk says where the first chunk is. */
inline bool parse_riff(const uint8_t* p, Info& info) {
    info = Info{};
    if (memcmp(p, "RIFF", 4) != 0 || memcmp(p + 8, "WAVE", 4) != 0)
        return false;
    info.riff_size = 8 + (uint32_t)(p[4] | (p[5] << 8) | (p[6] << 16) | ((uint32_t)p[7] << 24));
    info.next_chunk = 12;
    return true;
}

/* Checks what the chunks said and works out the frame count. Returns false if this is
 * not a WAV file the reader can handle. */
inline bool parse_finish(Info& info) {
    if (!info.data_start || !info.channels || !info.rate)
        return false;

    if (info.format == Info::PCM && info.bits >= 8 && info.bits <= 32 && info.bits % 8 == 0 && info.channels <= 8) {
        info.block_align = info.channels * (info.bits / 8);
        info.frames = info.data_size / info.block_align;
    } else if (info.format == Info::IMA_ADPCM && info.bits == 4 && info.channels <= 2 &&
               info.block_align > 4 * info.channels && info.block_align <= Info::max_adpcm_block &&
               (info.block_align - 4 * info.channels) % (4 * info.channels) == 0) {
        // Per channel a 4 byte header holding the first sample, then 4 bit codes in whole
        // groups of 4 bytes per channel.
        info.block_frames = 1 + (info.block_align - 4 * info.channels) * 2 / info.channels;
        // NB: a short last block is dropped (under 50 ms of the end). The encoder pads the
        // last block it writes; the "fact" chunk says how many frames are really audio.
        info.frames = info.data_size / info.block_align * info.block_frames;
        if (info.fact_frames && info.fact_frames < info.frames) info.frames = info.fact_frames;
    } else {
        return false;
    }
    return info.frames != 0;  // nothing to play is not a file to open
}

/* Reads the header of a WAV file: walks from chunk to chunk, in whatever order they come
 * and wherever in the file they are, until the data chunk. `read(position, dest, bytes)`
 * reads raw bytes at a position in the file and returns how many it got, or a negative
 * number on error. Returns false if this is not a WAV file the reader can handle. */
template <typename Read>
bool parse_file(uint32_t file_size, Info& info, Read&& read) {
    // Each chunk is read with enough of its content for the fields of "fmt ".
    uint8_t chunk[8 + 34];
    if (read(0u, chunk, 12u) < 12 || !parse_riff(chunk, info))
        return false;
    for (int chunks = 0; !info.data_start && info.next_chunk && chunks < 64; chunks++) {
        const uint32_t at = info.next_chunk;
        const int32_t got = read(at, chunk, (uint32_t)sizeof(chunk));
        if (got < 8) return false;
        take_chunk(chunk, got, at, file_size, info);
    }
    return parse_finish(info);
}

/* Step sizes of the IMA ADPCM decoder. The decoder does not look them up here: a lookup
 * in the SPI flash is as slow as a jump there (see ImaBlock). AdpcmFrames copies the
 * table into the buffer it is given, so it only takes up RAM while a file is decoded. */
constexpr uint16_t ima_step_size[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230,
    253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
    1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
    3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
    11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767};

/* Decodes one IMA ADPCM block of a WAV file, front to back, a run of frames at a time:
 * first the frame stored in the block header, then 8 frames per group of codes.
 *
 * Written for code that executes from the SPI flash, where every jump and every table
 * lookup in the flash stalls the CPU for about as long as a hundred instructions. So a
 * group is decoded in one straight line: no data dependent branches, no loop the
 * compiler cannot unroll, and the one table in RAM. */
class ImaBlock {
   public:
    static constexpr uint32_t run_frames = 8;

    // `steps` is the copy of ima_step_size to look the step sizes up in.
    void start(const uint8_t* block, uint16_t channels, const uint16_t* steps) {
        steps_ = steps;
        block_ = block;
        channels_ = channels;
        position_ = 0;
        for (uint16_t c = 0; c < channels; c++) {
            predictor_[c] = (int16_t)(block[4 * c] | (block[4 * c + 1] << 8));
            index_[c] = (block[4 * c + 2] > 88) ? 88 : block[4 * c + 2];
        }
    }

    // Number of the first frame the next run holds.
    uint32_t position() const { return position_; }

    // Decodes the next run into `frames` (room for run_frames frames) and returns how
    // many frames it holds. Always inlined, so that it runs wherever its caller does.
    __attribute__((always_inline)) uint32_t next_run(int16_t* frames) {
        if (position_ == 0) {
            frames[0] = predictor_[0];
            frames[channels_ - 1] = predictor_[channels_ - 1];
            position_ = 1;
            return 1;
        }

        // The codes come 8 per channel at a time: 4 bytes of the left channel, 4 of the
        // right, and so on.
        const uint8_t* group = block_ + 4 * channels_ + ((position_ - 1) / run_frames) * 4 * channels_;
        for (uint32_t c = 0; c < channels_; c++)
            decode_group(group + 4 * c, c, frames + c);
        position_ += run_frames;
        return run_frames;
    }

   private:
    // One copy for both channels, not inlined: it is the bulk of the decoder. Built for
    // speed so that the 8 samples are unrolled into one straight line.
    __attribute__((noinline, optimize("O3"))) void decode_group(const uint8_t* bytes, uint32_t channel, int16_t* out) {
        int32_t predictor = predictor_[channel];
        int32_t index = index_[channel];
        const uint32_t stride = channels_;
        uint32_t codes = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | ((uint32_t)bytes[3] << 24);

        // Low nibble first. Every "if" of the textbook decoder is a mask here:
        // -(bit) is all ones when the bit is set, x >> 31 is all ones when x is negative.
        for (uint32_t i = 0; i < run_frames; i++, codes >>= 4) {
            const int32_t step = steps_[index];
            int32_t delta = step >> 3;
            delta += step & -(int32_t)((codes >> 2) & 1);
            delta += (step >> 1) & -(int32_t)((codes >> 1) & 1);
            delta += (step >> 2) & -(int32_t)(codes & 1);
            const int32_t sign = -(int32_t)((codes >> 3) & 1);
            predictor += (delta ^ sign) - sign;

            // Clamp to 16 bits.
            const int32_t over = (32767 - predictor) >> 31;
            predictor = (predictor & ~over) | (32767 & over);
            const int32_t under = (predictor + 32768) >> 31;
            predictor = (predictor & ~under) | (-32768 & under);
            out[i * stride] = predictor;

            // The index moves by -1 for codes 0..3 and by 2, 4, 6, 8 for codes 4..7,
            // and stays within 0..88.
            const int32_t magnitude = codes & 7;
            const int32_t big = -(magnitude >> 2);
            index += (((magnitude - 3) * 2) & big) | (-1 & ~big);
            index &= ~(index >> 31);
            const int32_t above = index - 88;
            index = 88 + (above & (above >> 31));
        }

        predictor_[channel] = predictor;
        index_[channel] = index;
    }

    // No initializers: start() sets them all, and AdpcmFrames holds an array of these.
    const uint16_t* steps_;
    const uint8_t* block_;
    uint16_t channels_;
    uint16_t position_;
    int16_t predictor_[2];
    uint8_t index_[2];
};

/* Random access to the frames of an IMA ADPCM file.
 *
 * ADPCM only decodes forwards. To go back inside a block (reverse playback, or re-reading
 * the last frames) the decoder resumes from a checkpoint taken on the way through it,
 * instead of decoding the block again from its start. */
class AdpcmFrames {
   public:
    // Room for the step table, then one raw block.
    static constexpr size_t table_bytes = (sizeof(ima_step_size) + 3) & ~size_t{3};
    static constexpr size_t buffer_bytes(const Info& info) { return table_bytes + info.block_align + 3; }

    // `buffer` has to be buffer_bytes() long and word aligned.
    void start(const Info& info, uint8_t* buffer) {
        info_ = &info;
        steps_ = reinterpret_cast<uint16_t*>(buffer);
        memcpy(steps_, ima_step_size, sizeof(ima_step_size));
        buffer_ = buffer + table_bytes;
        block_number_ = UINT32_MAX;
    }

    // Decodes `frames` frames from `frame` on into `out`. `load(position, dest, bytes)`
    // reads raw bytes at a position in the file and returns how many it got, or a
    // negative number on error, which makes this return false.
    template <typename Load>
    bool read(uint32_t frame, uint32_t frames, int16_t* out, Load&& load) {
        const uint32_t channels = info_->channels;
        // Divide once, not per run: on this CPU a division is a library call.
        uint32_t number = frame / info_->block_frames;
        uint32_t offset = frame % info_->block_frames;
        while (frames) {
            if (number != block_number_) {
                // Placed in the buffer so that it lines up with its position in the file
                // (see WAVFileReader::read_at).
                const uint32_t position = info_->data_start + number * info_->block_align;
                uint8_t* const block = buffer_ + (position & 3);
                block_number_ = UINT32_MAX;  // until the new block is really there
                const int32_t got = load(position, block, (uint32_t)info_->block_align);
                if (got < 0) return false;
                // A short read (truncated file) decodes as a held sample, not as old data.
                if (got < info_->block_align)
                    memset(block + got, 0, info_->block_align - got);
                block_number_ = number;
                ima_.start(block, info_->channels, steps_);
                checkpoints_ = 0;
                run_length_ = 0;
            } else if (offset < run_start_) {
                ima_ = checkpoint_[checkpoint_of(offset)];
                run_length_ = 0;
            }

            // Runs up to the one holding the frame wanted, then copy what it holds.
            while (run_length_ == 0 || offset >= run_start_ + run_length_) {
                run_start_ = ima_.position();
                if (run_start_ <= 1 || (run_start_ - 1) % checkpoint_frames == 0) {
                    const uint32_t slot = checkpoint_of(run_start_);
                    if (slot == checkpoints_) checkpoint_[checkpoints_++] = ima_;
                }
                run_length_ = ima_.next_run(run_);
            }
            uint32_t n = run_start_ + run_length_ - offset;
            if (n > frames) n = frames;
            memcpy(out, &run_[(offset - run_start_) * channels], n * channels * sizeof(int16_t));
            out += n * channels;
            offset += n;
            if (offset == info_->block_frames) {
                offset = 0;
                number++;
            }
            frames -= n;
        }
        return true;
    }

   private:
    // Checkpoints sit at frame 0 and then every checkpoint_frames frames from frame 1 on,
    // which is where the runs of 8 start.
    static constexpr uint32_t checkpoint_frames = 512;
    static constexpr uint32_t checkpoint_of(uint32_t frame) { return frame ? 1 + (frame - 1) / checkpoint_frames : 0; }

    const Info* info_{nullptr};
    uint16_t* steps_{nullptr};  // the step table, at the start of the buffer given to start()
    uint8_t* buffer_{nullptr};  // one raw block, after it
    uint32_t block_number_{UINT32_MAX};
    ImaBlock ima_;
    ImaBlock checkpoint_[2 + Info::max_adpcm_frames / checkpoint_frames];
    uint32_t checkpoints_{0};
    int16_t run_[ImaBlock::run_frames * 2];  // the run decoded last...
    uint32_t run_start_{0};                  // ...the frame it starts at...
    uint32_t run_length_{0};                 // ...and how many frames it holds
};

}  // namespace wav

#endif /*__WAV_FORMAT_H__*/
