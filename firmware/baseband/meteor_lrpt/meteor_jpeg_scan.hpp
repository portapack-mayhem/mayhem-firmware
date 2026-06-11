/*
 * Copyright (C) 2026
 *
 * This file is part of PortaPack / Mayhem.
 *
 * Heuristic JPEG SOI scan for UI preview — Mayhem-original (not a SatDump file port).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef METEOR_LRPT_METEOR_JPEG_SCAN_HPP
#define METEOR_LRPT_METEOR_JPEG_SCAN_HPP

#include <cstddef>
#include <cstdint>

/* Scan CADU payload for JPEG SOI; fill up to `max_out` gray preview samples for UI. */
size_t meteor_scan_jpeg_preview_gray(const uint8_t* data, size_t len, uint8_t* gray_out, size_t max_out);

#endif
