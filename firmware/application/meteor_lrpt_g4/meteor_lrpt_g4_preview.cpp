/*
 * Copyright (C) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "meteor_lrpt_g4_preview.hpp"

#include "bmpfile.hpp"
#include "ui.hpp"

#include "hal.h"

namespace meteor_lrpt_g4 {

bool publish_decoded_bmp_preview(
    SharedMemory::MeteorLrptG4Ipc& ipc,
    const std::filesystem::path& bmp_path) {
    BMPFile bmp{};
    if (!bmp.open(bmp_path, true))
        return false;

    const uint32_t iw = bmp.get_width();
    const uint32_t ih = bmp.get_real_height();
    if (iw < 2 || ih < 2) {
        bmp.close();
        return false;
    }

    constexpr uint16_t pw = (uint16_t)SharedMemory::MeteorLrptG4Ipc::kPreviewWidth;
    constexpr uint16_t ph = (uint16_t)SharedMemory::MeteorLrptG4Ipc::kPreviewHeight;

    for (unsigned py = 0; py < ph; py++) {
        for (unsigned px = 0; px < pw; px++) {
            const uint32_t sx = (px * (iw - 1u)) / (pw - 1u);
            const uint32_t sy = (py * (ih - 1u)) / (ph - 1u);
            if (!bmp.seek(sx, sy)) {
                bmp.close();
                return false;
            }
            ui::Color c{};
            if (!bmp.read_next_px(c, true)) {
                bmp.close();
                return false;
            }
            const size_t o = ((size_t)py * pw + (size_t)px) * 3u;
            ipc.preview_rgb[o + 0] = c.r();
            ipc.preview_rgb[o + 1] = c.g();
            ipc.preview_rgb[o + 2] = c.b();
        }
    }
    bmp.close();

    ipc.preview_width = pw;
    ipc.preview_height = ph;
    __DMB();
    ipc.preview_seq = ipc.preview_seq + 1u;
    __DMB();
    return true;
}

}  // namespace meteor_lrpt_g4
