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

namespace meteor_lrpt_g4 {

/** Hard cap for LRPT MSU-MR product JPEG dimensions (TJpgDec + BMP on M0). */
constexpr unsigned kG4MaxJpegWidth = 2048;
constexpr unsigned kG4MaxJpegHeight = 2048;

/** Quick scan for progressive SOF (0xFFC2) — tjpgd does not support progressive JPEG. */
bool jpeg_buffer_has_progressive_sof(const uint8_t* jpg, size_t len);

/**
 * Decode baseline JPEG to a new 24 bpp BMP at `path` (overwrites if present).
 * Runs `jd_prepare` before `BMPFile::create` so width matches the SOF; the BMP grows row-by-row
 * (`expand_y`) as TJpgDec emits MCUs (MCU order matches `jd_decomp` scan).
 * If `ppm_path` is non-null and non-empty, also writes a binary P6 RGB sidecar (same dimensions).
 */
bool decode_jpeg_to_new_bmp_file(
    const uint8_t* jpg,
    size_t len,
    const std::filesystem::path& path,
    uint32_t& drop_bits,
    uint8_t& jresult_out,
    const std::filesystem::path* ppm_path = nullptr);

}  // namespace meteor_lrpt_g4

#endif
