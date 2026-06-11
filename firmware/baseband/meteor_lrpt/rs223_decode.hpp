/*
 * Copyright (C) 2026
 *
 * This file is part of PortaPack / Mayhem.
 *
 * Interleave depth / call pattern matches SatDump `reedsolomon::ReedSolomon::decode_interlaved`
 * (`src-core/common/codings/reedsolomon/reedsolomon.cpp`) as used from
 * `plugins/meteor_support/meteor/module_meteor_lrpt_decoder.cpp`. RS byte core: libfec (LGPL).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Upstream mapping: `SATDUMP_VENDOR.md`.
 */
#ifndef __METEOR_LRPT_RS223_DECODE_HPP__
#define __METEOR_LRPT_RS223_DECODE_HPP__

#include <cstdint>

/* Depth-4 interleaved RS(255,223) over 1020 bytes (SatDump `decode_interlaved(..., false, 4, ...)`).
 * Corrects `body1020` in place on success.
 * `errors_out4[b]` = number of corrected bytes among the first 223 symbols of codeword b, or -1 if uncorrectable.
 * Returns true iff all four codewords decode successfully. */
bool meteor_lrpt_rs_decode_interleaved_depth4(uint8_t* body1020, int16_t* errors_out4);

#endif /* __METEOR_LRPT_RS223_DECODE_HPP__ */
