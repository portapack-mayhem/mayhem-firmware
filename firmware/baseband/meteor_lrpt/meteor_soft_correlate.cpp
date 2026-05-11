/*
 * Copyright (C) 2026
 *
 * This file is part of PortaPack / Mayhem (GPL v2 or later).
 *
 * QPSK correlator / rotate_soft logic is derived from SatDump
 * `src-core/common/codings/correlator.cpp` and `rotation.cpp` (MIT).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Upstream mapping: `SATDUMP_VENDOR.md`.
 */
#include "meteor_soft_correlate.hpp"

#include <algorithm>
#include <array>
#include <cstring>

namespace meteor_lrpt {
namespace {

constexpr int kCorrStrong = 45;

static uint64_t rotate_64(uint64_t word, unsigned p) {
    uint64_t i = word & 0xaaaaaaaaaaaaaaaaULL;
    uint64_t q = word & 0x5555555555555555ULL;

    switch (p) {
        case 0:
            break;
        case 1: /* PHASE_90 */
            word = ((i ^ 0xaaaaaaaaaaaaaaaaULL) >> 1) | (q << 1);
            break;
        case 2: /* PHASE_180 */
            word = word ^ 0xffffffffffffffffULL;
            break;
        case 3: /* PHASE_270 */
            word = (i >> 1) | ((q ^ 0x5555555555555555ULL) << 1);
            break;
        default:
            break;
    }

    return ((word & 0x5555555555555555ULL) << 1) | ((word & 0xAAAAAAAAAAAAAAAAULL) >> 1);
}

static int corr_64(uint64_t v1, uint64_t v2) {
    int cor = 0;
    uint64_t diff = v1 ^ v2;
    for (; diff; cor++)
        diff &= diff - 1;
    return 64 - cor;
}

static uint64_t swap_iq_u64(uint64_t in) {
    uint64_t i = in & 0xaaaaaaaaaaaaaaaaULL;
    uint64_t q = in & 0x5555555555555555ULL;
    return (i >> 1) | (q << 1);
}

static void build_syncwords(uint64_t syncword, std::array<uint64_t, 8>& syncwords) {
    for (unsigned i = 0; i < 4; i++)
        syncwords[i] = rotate_64(syncword, i);
    for (unsigned i = 4; i < 8; i++)
        syncwords[i] = rotate_64((swap_iq_u64(syncword) ^ 0xffffffffffffffffULL), i - 4);
}

static void rotate_soft_pairs(int8_t* soft, size_t nbytes, unsigned phase, bool iqswap) {
    for (size_t i = 0; i < nbytes; i++)
        if (soft[i] == -128)
            soft[i] = -127;

    if (iqswap) {
        for (size_t i = 0; i + 1 < nbytes; i += 2) {
            int8_t x = soft[i + 1];
            soft[i + 1] = soft[i];
            soft[i] = x;
        }
    }

    int8_t tmp = 0;
    switch (phase) {
        case 0:
            break;
        case 1:
            for (; nbytes >= 2; nbytes -= 2) {
                tmp = *soft;
                *soft = *(soft + 1);
                *(soft + 1) = (int8_t)-tmp;
                soft += 2;
            }
            break;
        case 2:
            for (; nbytes > 0; nbytes--) {
                *soft = (int8_t)-*soft;
                soft++;
            }
            break;
        case 3:
            for (; nbytes >= 2; nbytes -= 2) {
                tmp = *soft;
                *soft = (int8_t)-*(soft + 1);
                *(soft + 1) = tmp;
                soft += 2;
            }
            break;
        default:
            break;
    }
}

}  // namespace

void correlate_rotate_qpsk_legacy(int8_t* soft_io, size_t length, bool diff_decode, CorrQpskResult* out) {
    if (!out || length < 64 || (length % 8) != 0)
        return;

    *out = CorrQpskResult{};

    const uint64_t syncword = diff_decode ? kLrptSyncDiff : kLrptSyncNoDiff;
    std::array<uint64_t, 8> syncwords{};
    build_syncwords(syncword, syncwords);

    const int ilen = (int)length;
    std::array<uint8_t, 16384 / 8> hard_buf{}; /* 2048 for max block */
    if ((size_t)ilen / 8 > hard_buf.size())
        return;

    int bits = 0;
    int bytes = 0;
    uint8_t shifter = 0;
    for (int i = 0; i < ilen; i++) {
        shifter = (uint8_t)((shifter << 1) | (soft_io[i] > 0 ? 1 : 0));
        bits++;
        if (bits == 8) {
            hard_buf[(size_t)bytes] = shifter;
            bits = 0;
            bytes++;
        }
    }

    uint64_t current = ((uint64_t)hard_buf[0] << 56) | ((uint64_t)hard_buf[1] << 48) | ((uint64_t)hard_buf[2] << 40) |
                       ((uint64_t)hard_buf[3] << 32) | ((uint64_t)hard_buf[4] << 24) | ((uint64_t)hard_buf[5] << 16) |
                       ((uint64_t)hard_buf[6] << 8) | ((uint64_t)hard_buf[7] << 0);

    int correlation = 0;
    int offset = 0;
    int pos = 8;

    for (unsigned p = 0; p < 8; p++) {
        int corr = corr_64(syncwords[p], current);
        if (corr > kCorrStrong) {
            out->align_skip = 0;
            out->score = corr;
            out->locked = true;
            out->phase = p % 4;
            out->swap = (p / 4) == 0;
            rotate_soft_pairs(soft_io, length, out->phase, out->swap);
            return;
        }
    }

    const int outer_lim = (ilen / 8) - 8;
    for (int i = 0; i < outer_lim; i++) {
        for (int ii = 0; ii < 8; ii += 2) {
            for (unsigned p = 0; p < 8; p++) {
                int corr = corr_64(syncwords[p], current);
                if (corr > correlation) {
                    correlation = corr;
                    offset = i * 8 + ii;
                    out->phase = p % 4;
                    out->swap = (p / 4) == 0;
                }
            }
            current = (current << 2) | ((uint64_t)(hard_buf[(size_t)pos] >> (6 - ii)) & 3u);
        }
        pos++;
    }

    out->score = correlation;
    out->locked = (offset == 0 && correlation > kCorrStrong);
    out->align_skip = (size_t)std::max(0, offset);

    if (offset > 0 && (size_t)offset < length) {
        std::memmove(soft_io, soft_io + offset, length - (size_t)offset);
        std::memset(soft_io + length - (size_t)offset, 0, (size_t)offset);
    }

    rotate_soft_pairs(soft_io, length, out->phase, out->swap);
}

}  // namespace meteor_lrpt
