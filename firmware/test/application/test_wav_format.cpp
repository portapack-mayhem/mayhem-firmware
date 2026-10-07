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

#include "doctest.h"
#include "wav_format.hpp"

using namespace wav;

/* A 48 kHz stereo IMA ADPCM WAV with 64 byte blocks, cut after its first two blocks:
 *   ffmpeg -f lavfi -i "aevalsrc=0.6*sin(2*PI*700*t)|0.4*sin(2*PI*1900*t+1):s=48000:d=0.01" \
 *          -map_metadata -1 -fflags +bitexact -flags:a +bitexact -c:a adpcm_ima_wav -block_size 64 v.wav
 * and what ffmpeg decodes those two blocks to (ffmpeg -i v.wav -f s16le v.raw). */
static const uint8_t adpcm_wav[] = {
    82,
    73,
    70,
    70,
    116,
    2,
    0,
    0,
    87,
    65,
    86,
    69,
    102,
    109,
    116,
    32,
    20,
    0,
    0,
    0,
    17,
    0,
    2,
    0,
    128,
    187,
    0,
    0,
    134,
    210,
    0,
    0,
    64,
    0,
    4,
    0,
    2,
    0,
    57,
    0,
    102,
    97,
    99,
    116,
    4,
    0,
    0,
    0,
    224,
    1,
    0,
    0,
    100,
    97,
    116,
    97,
    64,
    2,
    0,
    0,
    0,
    0,
    0,
    0,
    21,
    43,
    0,
    0,
    119,
    119,
    119,
    119,
    119,
    119,
    255,
    255,
    39,
    0,
    0,
    0,
    175,
    153,
    153,
    8,
    8,
    136,
    137,
    154,
    33,
    67,
    67,
    51,
    186,
    188,
    203,
    188,
    34,
    1,
    185,
    205,
    187,
    173,
    203,
    186,
    219,
    186,
    171,
    153,
    186,
    187,
    187,
    170,
    32,
    83,
    68,
    51,
    154,
    136,
    33,
    99,
    51,
    19,
    144,
    218,
    254,
    188,
    49,
    0,
    243,
    25,
    60,
    0,
    68,
    52,
    68,
    51,
    189,
    188,
    171,
    138,
    52,
    52,
    67,
    51,
    24,
    83,
    68,
    51,
    67,
    50,
    35,
    35,
    67,
    18,
    128,
    186,
    34,
    17,
    136,
    170,
    189,
    189,
    203,
    154,
    205,
    219,
    188,
    188,
    137,
    33,
    83,
    52,
    204,
    202,
    186,
    188,
    52,
    35,
    2,
    152,
    202,
    186,
    186,
    171,
    219,
    188,
    173,
    187,
};

static const int16_t adpcm_decoded[] = {
    0,
    11029,
    11,
    11040,
    41,
    11070,
    104,
    11133,
    240,
    11269,
    533,
    10976,
    1164,
    10345,
    2521,
    8988,
    5431,
    6078,
    11667,
    -158,
    16124,
    -4615,
    16934,
    -7046,
    17670,
    -9255,
    18339,
    -11263,
    18947,
    -13088,
    19500,
    -13641,
    20003,
    -13138,
    19546,
    -11766,
    19961,
    -9688,
    19583,
    -7042,
    19240,
    -3950,
    18304,
    -1041,
    18020,
    2361,
    16729,
    5563,
    16026,
    8472,
    14960,
    10362,
    13602,
    12079,
    12015,
    13015,
    10523,
    13299,
    9165,
    12525,
    7578,
    10883,
    5658,
    8537,
    3851,
    5726,
    2209,
    3080,
    717,
    -699,
    -1417,
    -3215,
    -2837,
    -6417,
    -4644,
    -9326,
    -6756,
    -11216,
    -8176,
    -12246,
    -9983,
    -13182,
    -11156,
    -12898,
    -12648,
    -11607,
    -14006,
    -9965,
    -15239,
    -7619,
    -16360,
    -4808,
    -17379,
    -1406,
    -18041,
    1796,
    -18642,
    4705,
    -19189,
    7351,
    -19487,
    9755,
    -19577,
    11940,
    -19659,
    12792,
    -19436,
    13050,
    -19096,
    12347,
    -18665,
    11281,
    -17936,
    9147,
    -17154,
    6643,
    -16259,
    3519,
    -15176,
    610,
    -13865,
    -2792,
    -12632,
    -5994,
    -11190,
    -8903,
    -9444,
    -10793,
    -7802,
    -12510,
    -6310,
    -12822,
    -4564,
    -13106,
    -2922,
    -12332,
    -1002,
    -10690,
    805,
    -8344,
    2447,
    -5533,
    4367,
    -2131,
    6174,
    1071,
    7816,
    3980,
    9308,
    6626,
    11054,
    9718,
    12227,
    11796,
    13719,
    12930,
    15077,
    13273,
    15958,
    12961,
    17079,
    11541,
    17807,
    9734,
    18469,
    7153,
    19070,
    4749,
    19398,
    1314,
    19696,
    -1888,
    19606,
    -4797,
    19524,
    -8199,
    19151,
    -10486,
    18811,
    -11732,
    18133,
    -12866,
    17319,
    -13209,
    16553,
    -12273,
    15459,
    -10853,
    14148,
    -9046,
    12915,
    -6465,
    11473,
    -3373,
    10115,
    -464,
    8528,
    2938,
    6608,
    6140,
    5317,
    9049,
    3205,
    10939,
    1785,
    12656,
    -22,
    12968,
    -2134,
    12684,
    -4122,
    11910,
    -5413,
    10268,
    -7525,
    7922,
    -8945,
    5111,
    -10752,
    2465,
    -11925,
    -1314,
    -13417,
    -3830,
    -14775,
    -7032,
    -15656,
    -9941,
};

/* parse_file() on a file of which only the first `size` bytes exist in memory; the rest
 * (audio data nobody looks at) is made up from `file_size`. */
static bool parse(const uint8_t* p, size_t size, uint32_t file_size, Info& info) {
    return parse_file(file_size, info, [p, size](uint32_t position, uint8_t* dest, uint32_t bytes) -> int32_t {
        if (position >= size) return 0;
        const uint32_t n = (bytes < size - position) ? bytes : size - position;
        memcpy(dest, p + position, n);
        return n;
    });
}

TEST_SUITE_BEGIN("wav_format");

TEST_CASE("parse should find the format and data of an ADPCM file.") {
    Info info;
    REQUIRE(parse(adpcm_wav, sizeof(adpcm_wav), sizeof(adpcm_wav), info));
    CHECK_EQ(info.format, Info::IMA_ADPCM);
    CHECK_EQ(info.channels, 2);
    CHECK_EQ(info.rate, 48000);
    CHECK_EQ(info.block_align, 64);
    CHECK_EQ(info.block_frames, 57);
    CHECK_EQ(info.data_start, 60);
    CHECK_EQ(info.frames, 2 * 57);  // the data chunk is cut short by the end of the file
}

TEST_CASE("parse should reject what the reader can't handle.") {
    Info info;
    CHECK_FALSE(parse(adpcm_wav, 8, sizeof(adpcm_wav), info));

    uint8_t no_rate[sizeof(adpcm_wav)];
    memcpy(no_rate, adpcm_wav, sizeof(adpcm_wav));
    memset(&no_rate[24], 0, 4);
    CHECK_FALSE(parse(no_rate, sizeof(no_rate), sizeof(no_rate), info));

    // A stereo block has to hold whole groups of 4 code bytes per channel.
    uint8_t odd_block[sizeof(adpcm_wav)];
    memcpy(odd_block, adpcm_wav, sizeof(adpcm_wav));
    odd_block[32] = 12;
    CHECK_FALSE(parse(odd_block, sizeof(odd_block), sizeof(odd_block), info));

    // A chunk claiming to be larger than the file ends the search instead of derailing it.
    uint8_t huge_chunk[sizeof(adpcm_wav)];
    memcpy(huge_chunk, adpcm_wav, sizeof(adpcm_wav));
    memset(&huge_chunk[16], 0xFF, 4);
    huge_chunk[16] = 0xF8;
    CHECK_FALSE(parse(huge_chunk, sizeof(huge_chunk), sizeof(huge_chunk), info));
}

/* A canonical 44 byte PCM header; the data itself is never looked at by parse_file(). */
static void pcm_header(uint8_t* p, uint16_t channels, uint32_t rate, uint16_t bits, uint32_t data_size) {
    const auto u16 = [p](size_t at, uint16_t v) { p[at] = v; p[at + 1] = v >> 8; };
    const auto u32 = [p](size_t at, uint32_t v) { p[at] = v; p[at + 1] = v >> 8; p[at + 2] = v >> 16; p[at + 3] = v >> 24; };
    memcpy(p, "RIFF", 4);
    u32(4, 36 + data_size);
    memcpy(p + 8, "WAVEfmt ", 8);
    u32(16, 16);
    u16(20, Info::PCM);
    u16(22, channels);
    u32(24, rate);
    u32(28, rate * channels * bits / 8);
    u16(32, channels * bits / 8);
    u16(34, bits);
    memcpy(p + 36, "data", 4);
    u32(40, data_size);
}

TEST_CASE("parse should count the frames of PCM files of any rate and width.") {
    uint8_t header[44];
    Info info;

    pcm_header(header, 2, 48000, 16, 4000);
    REQUIRE(parse(header, sizeof(header), 44 + 4000, info));
    CHECK_EQ(info.format, Info::PCM);
    CHECK_EQ(info.channels, 2);
    CHECK_EQ(info.rate, 48000);
    CHECK_EQ(info.bits, 16);
    CHECK_EQ(info.block_align, 4);
    CHECK_EQ(info.data_start, 44);
    CHECK_EQ(info.frames, 1000);

    pcm_header(header, 1, 8000, 8, 4000);
    REQUIRE(parse(header, sizeof(header), 44 + 4000, info));
    CHECK_EQ(info.rate, 8000);
    CHECK_EQ(info.bits, 8);
    CHECK_EQ(info.block_align, 1);
    CHECK_EQ(info.frames, 4000);

    pcm_header(header, 1, 44100, 16, 4000);
    REQUIRE(parse(header, sizeof(header), 44 + 4000, info));
    CHECK_EQ(info.frames, 2000);
}

TEST_CASE("parse should read the real format of a WAVE_FORMAT_EXTENSIBLE file.") {
    // What ffmpeg writes for 24 bit or for more than 48 kHz: a 40 byte fmt chunk whose
    // format tag is 0xFFFE, with the PCM tag at the start of the sub format GUID.
    uint8_t header[68]{};
    pcm_header(header, 2, 96000, 16, 0);
    header[16] = 40;
    header[20] = 0xFE;
    header[21] = 0xFF;
    header[36] = 22;  // cbSize
    header[44] = Info::PCM;
    memcpy(&header[60], "data", 4);
    header[64] = 0xA0;  // 4000 bytes
    header[65] = 0x0F;

    Info info;
    REQUIRE(parse(header, sizeof(header), 68 + 4000, info));
    CHECK_EQ(info.format, Info::PCM);
    CHECK_EQ(info.rate, 96000);
    CHECK_EQ(info.data_start, 68);
    CHECK_EQ(info.frames, 1000);
}

TEST_CASE("parse_file should find a data chunk that lies far into the file.") {
    // fmt, then a 600 byte LIST chunk, then data.
    uint8_t file[44 + 608 + 8]{};
    pcm_header(file, 1, 48000, 16, 0);
    memcpy(&file[36], "LIST", 4);
    file[40] = 0x58;  // 600
    file[41] = 0x02;
    memcpy(&file[44 + 600], "data", 4);
    file[44 + 604] = 0xA0;  // 4000 bytes
    file[44 + 605] = 0x0F;
    const uint32_t file_size = 44 + 608 + 4000;

    Info info;
    REQUIRE(parse(file, sizeof(file), file_size, info));
    CHECK_EQ(info.data_start, 652);
    CHECK_EQ(info.frames, 2000);
}

TEST_CASE("parse should reject channel counts and widths that make no sense.") {
    uint8_t header[44];
    Info info;
    pcm_header(header, 2, 48000, 16, 4000);
    header[22] = 0x00;  // 32768 channels
    header[23] = 0x80;
    CHECK_FALSE(parse(header, sizeof(header), 44 + 4000, info));

    pcm_header(header, 2, 48000, 16, 4000);
    header[34] = 64;  // bits
    CHECK_FALSE(parse(header, sizeof(header), 44 + 4000, info));
}

TEST_CASE("parse should reject a file with nothing to play.") {
    uint8_t header[44];
    Info info;
    pcm_header(header, 1, 48000, 16, 0);
    CHECK_FALSE(parse(header, sizeof(header), 44, info));

    // An ADPCM file cut off inside its first block.
    CHECK_FALSE(parse(adpcm_wav, 60 + 40, 60 + 40, info));
}

TEST_CASE("parse should take the frame count of an ADPCM file from its fact chunk.") {
    // The whole file has 9 blocks of 57 frames, but the encoder padded the last one:
    // the fact chunk says 480 frames are audio.
    Info info;
    REQUIRE(parse(adpcm_wav, sizeof(adpcm_wav), 60 + 9 * 64, info));
    CHECK_EQ(info.fact_frames, 480);
    CHECK_EQ(info.frames, 480);
}

TEST_CASE("parse should not count data the file does not hold.") {
    uint8_t header[44];
    Info info;
    pcm_header(header, 1, 48000, 16, 4000);
    REQUIRE(parse(header, sizeof(header), 44 + 1000, info));
    CHECK_EQ(info.data_size, 1000);
    CHECK_EQ(info.frames, 500);
}

TEST_CASE("ImaBlock should decode the same samples as ffmpeg.") {
    Info info;
    REQUIRE(parse(adpcm_wav, sizeof(adpcm_wav), sizeof(adpcm_wav), info));

    // Run by run: the header frame, then 8 frames per group of codes.
    for (uint32_t number = 0; number < 2; number++) {
        ImaBlock block;
        block.start(&adpcm_wav[info.data_start + number * info.block_align], info.channels, ima_step_size);
        const int16_t* want = &adpcm_decoded[number * info.block_frames * 2];
        uint32_t frame = 0;
        while (frame < info.block_frames) {
            CHECK_EQ(block.position(), frame);
            int16_t run[ImaBlock::run_frames * 2];
            const uint32_t n = block.next_run(run);
            REQUIRE_EQ(n, frame == 0 ? 1 : ImaBlock::run_frames);
            for (uint32_t i = 0; i < n * 2; i++)
                CHECK_EQ(run[i], want[frame * 2 + i]);
            frame += n;
        }
        CHECK_EQ(frame, info.block_frames);
    }
}

TEST_CASE("AdpcmFrames should give the same frames in any order.") {
    Info info;
    REQUIRE(parse(adpcm_wav, sizeof(adpcm_wav), sizeof(adpcm_wav), info));

    alignas(4) uint8_t buffer[AdpcmFrames::table_bytes + 64 + 3];
    REQUIRE(AdpcmFrames::buffer_bytes(info) <= sizeof(buffer));
    AdpcmFrames frames;
    frames.start(info, buffer);
    int loads = 0;
    const auto load = [&loads](uint32_t position, uint8_t* dest, uint32_t bytes) -> int32_t {
        memcpy(dest, &adpcm_wav[position], bytes);
        loads++;
        return bytes;
    };

    // Everything at once, across the block boundary.
    int16_t all[2 * 57 * 2];
    REQUIRE(frames.read(0, 2 * 57, all, load));
    CHECK_EQ(memcmp(all, adpcm_decoded, sizeof(all)), 0);
    CHECK_EQ(loads, 2);

    // Backwards a frame at a time, as reverse playback does: every frame still matches,
    // and going back inside a block does not fetch it again.
    loads = 0;
    for (int frame = 2 * 57 - 1; frame >= 0; frame--) {
        int16_t got[2];
        REQUIRE(frames.read(frame, 1, got, load));
        CHECK_EQ(got[0], adpcm_decoded[frame * 2]);
        CHECK_EQ(got[1], adpcm_decoded[frame * 2 + 1]);
    }
    CHECK_EQ(loads, 1);

    // A failed load is reported.
    CHECK_FALSE(frames.read(60, 1, all, [](uint32_t, uint8_t*, uint32_t) -> int32_t { return -1; }));
}

TEST_SUITE_END();
