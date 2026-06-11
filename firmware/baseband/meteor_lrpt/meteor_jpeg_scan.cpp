/*
 * Copyright (C) 2026
 *
 * This file is part of PortaPack / Mayhem.
 *
 * Heuristic preview only — Mayhem-original (see `SATDUMP_VENDOR.md` “Mayhem-only”).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "meteor_jpeg_scan.hpp"

#include <cstring>

size_t meteor_scan_jpeg_preview_gray(const uint8_t* data, size_t len, uint8_t* gray_out, size_t max_out) {
    if (!data || !gray_out || max_out == 0)
        return 0;
    size_t w = 0;
    for (size_t i = 0; i + 1 < len && w < max_out; i++) {
        if (data[i] == 0xff && data[i + 1] == 0xd8) {
            gray_out[w++] = 255;
            continue;
        }
        if (w > 0)
            gray_out[w++] = (uint8_t)((data[i] ^ (i >> 3)) & 0xff);
        if (w >= max_out)
            break;
    }
    if (w == 0 && len > 0) {
        gray_out[0] = (uint8_t)(data[0] ^ data[len / 2]);
        w = 1;
    }
    return w;
}
