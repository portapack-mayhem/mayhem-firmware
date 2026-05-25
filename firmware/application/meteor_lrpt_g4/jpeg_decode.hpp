/*
 * Copyright (C) 2026
 *
 * Tiny JPEG decode for G4 (TJpgDec / ChaN), RGB888 → BMP row writes.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef APPLICATION_METEOR_LRPT_G4_JPEG_DECODE_HPP
#define APPLICATION_METEOR_LRPT_G4_JPEG_DECODE_HPP

#include <cstddef>
#include <cstdint>

#include "file.hpp"

namespace SharedMemory {
struct MeteorLrptG4Ipc;
}

namespace meteor_lrpt_g4 {

/** Hard cap for LRPT MSU-MR product JPEG dimensions (TJpgDec + BMP on M0). */
constexpr unsigned kG4MaxJpegWidth = 2048;
constexpr unsigned kG4MaxJpegHeight = 2048;

/** Quick scan for progressive SOF (0xFFC2) — tjpgd does not support progressive JPEG. */
bool jpeg_buffer_has_progressive_sof(const uint8_t* jpg, size_t len);

/**
 * Decode baseline JPEG to a new 24 bpp BMP at `path` (overwrites if present).
 * When `g4_ipc` is non-null, fills `preview_rgb` during decode (no second BMP read).
 */
bool decode_jpeg_to_new_bmp_file(
    const uint8_t* jpg,
    size_t len,
    const std::filesystem::path& path,
    uint32_t& drop_bits,
    uint8_t& jresult_out,
    SharedMemory::MeteorLrptG4Ipc* g4_ipc = nullptr);  // optional live preview IPC (main firmware only)

}  // namespace meteor_lrpt_g4

#endif
