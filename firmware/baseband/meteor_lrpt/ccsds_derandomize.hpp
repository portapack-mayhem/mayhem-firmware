/*
 * CCSDS randomization (byte-wise PN) — table and API aligned with SatDump
 * `src-core/common/codings/randomization.cpp` (MIT).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Upstream mapping: `SATDUMP_VENDOR.md`.
 */
#ifndef METEOR_LRPT_CCSDS_DERANDOMIZE_HPP
#define METEOR_LRPT_CCSDS_DERANDOMIZE_HPP

#include <cstddef>
#include <cstdint>

void meteor_derand_ccsds(uint8_t* data, size_t length);

#endif
