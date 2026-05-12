/*
 * Copyright (C) 2026
 *
 * Downscale last decoded BMP into `MeteorLrptG4Ipc` shared preview (M0 UI thread polls).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef APPLICATION_METEOR_LRPT_G4_METEOR_LRPT_G4_PREVIEW_HPP
#define APPLICATION_METEOR_LRPT_G4_METEOR_LRPT_G4_PREVIEW_HPP

#include "portapack_shared_memory.hpp"

#include "file.hpp"

namespace meteor_lrpt_g4 {

/** Fill `preview_rgb` / dimensions and bump `preview_seq` (with DMB). Returns false if BMP unreadable. */
bool publish_decoded_bmp_preview(
    SharedMemory::MeteorLrptG4Ipc& ipc,
    const std::filesystem::path& bmp_path);

}  // namespace meteor_lrpt_g4

#endif
