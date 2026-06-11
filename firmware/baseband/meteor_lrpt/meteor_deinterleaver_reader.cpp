/*
 * Copyright (C) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "meteor_deinterleaver_reader.hpp"

#include "meteor_lrpt_deinterleave_dispatch.hpp"
#include "meteor_soft_correlate.hpp"

#include <algorithm>
#include <cstring>

namespace meteor_lrpt {
namespace {

constexpr uint8_t kSyncwords[] = {0x27, 0x4E, 0xD8, 0xB1};

int popcount8(uint8_t v) {
    int c = 0;
    while (v) {
        v = (uint8_t)(v & (uint8_t)(v - 1));
        c++;
    }
    return c;
}

static void soft_to_hard(uint8_t* hard, const int8_t* soft, int len_soft) {
    while (len_soft > 0) {
        uint8_t b = 0;
        for (int i = 7; i >= 0; i--)
            b |= (uint8_t)((*soft++ < 0) ? (1u << (unsigned)i) : 0);
        *hard++ = b;
        len_soft -= 8;
    }
}

}  // namespace

int MeteorDeinterleaverReader::autocorrelate(unsigned* rotation_out, int period_bytes, uint8_t* hard, int len_bytes) {
    /* On-device path only uses `period_bytes == kInterMarkerStride/8` (10). */
    constexpr int kMaxPeriodBytes = kInterMarkerStride / 8;
    if (period_bytes <= 0 || period_bytes > kMaxPeriodBytes)
        return 0;

    int len = len_bytes;
    len -= len % period_bytes;
    if (len <= 0)
        return 0;

    const int ones_sz = 8 * period_bytes;
    std::array<int, 8 * kMaxPeriodBytes> ones_count{};
    std::array<int, 8 * kMaxPeriodBytes + 8> average_bit{};

    int i, j, k;
    uint8_t tmp, xo, window;

    for (i = 0; i < period_bytes; i++) {
        j = len - period_bytes + i - 1;
        tmp = hard[j];
        for (j -= period_bytes; j >= 0; j -= period_bytes) {
            xo = (uint8_t)(hard[j] ^ tmp);
            tmp = hard[j];
            hard[j] = xo;
            for (k = 0; k < 8; k++)
                average_bit[(size_t)(8 * i + 7 - k)] += (tmp & (1u << (unsigned)k)) ? 1 : -1;
        }
    }

    window = 0;
    /* `hard` points one past the last byte of the XOR-diff buffer; the loop treats it as a sliding
     * 8-bit window over the preceding bytes (SatDump `DeinterleaverReader` parity). Do not reorder. */
    hard--;
    for (i = 0; i < 8 * (len - period_bytes); i++) {
        if (!(i % 8))
            hard++;
        window = (uint8_t)((window >> 1) | ((*hard << (i % 8)) & 0x80));
        ones_count[(size_t)(i % (8 * period_bytes))] += popcount8(window);
    }

    int best_idx = 0;
    int best_corr = ones_count[0] - len / 64;
    for (i = 1; i < ones_sz; i++) {
        if (ones_count[(size_t)i] < best_corr) {
            best_corr = ones_count[(size_t)i];
            best_idx = i;
        }
    }

    tmp = 0;
    for (i = 7; i >= 0; i--)
        tmp |= (uint8_t)((average_bit[(size_t)(best_idx + i)] > 0) ? (1u << (unsigned)i) : 0);

    *rotation_out = 0;
    int best_h = popcount8((uint8_t)(tmp ^ kSyncwords[0]));
    for (i = 1; i < 4; i++) {
        const int c = popcount8((uint8_t)(tmp ^ kSyncwords[i]));
        if (best_h > c) {
            best_h = c;
            *rotation_out = (unsigned)i;
        }
    }

    return best_idx;
}

MeteorDeinterleaverReader::MeteorDeinterleaverReader(IMeteorDeintRing& ring, uint8_t* hard_scratch, MeteorDeintFileRing* file_ring)
    : ring_{ring},
      file_ring_{file_ring},
      hard_scratch_{hard_scratch} {
}

int MeteorDeinterleaverReader::read_samples(MeteorDeintStreamRead read, void* ctx, int8_t* dst, size_t len) {
    if (!hard_scratch_)
        return 1;
    if (len > kDeinterleaverReaderMaxOutputLen)
        return 1;

    const size_t num_samples = meteor_deinterleave_num_samples(len, ring_branch_);
    const size_t hard_bytes = inter_size_bytes(len);
    if (hard_bytes > kDeinterleaverReaderHardScratchBytes)
        return 1;

    uint8_t* const hard = hard_scratch_;

    const int off0 = stream_carry_;
    if (off0) {
        const size_t n = std::min((size_t)off0, num_samples);
        std::memcpy(dst, from_prev_.data(), n);
        if ((size_t)off0 > n)
            std::memmove(from_prev_.data(), from_prev_.data() + n, (size_t)off0 - n);
    }
    if (num_samples > (size_t)off0 && !read(ctx, dst + (size_t)off0, num_samples - (size_t)off0))
        return 1;
    stream_carry_ = off0 - (int)std::min((size_t)off0, num_samples);

    if (num_samples < (size_t)(kInterMarkerStride * 8)) {
        rotate_soft_satdump(dst, num_samples, rotation_, false);
        const int dr = meteor_lrpt_deinterleave_dispatch(ring_, file_ring_, dst, dst, len, ring_offset_, ring_branch_);
        if (dr != 0)
            return dr;
    } else {
        const int ns8 = (int)(num_samples & ~(size_t)7u);
        soft_to_hard(hard, dst, ns8);

        int sync_offs = autocorrelate(&rotation_, kInterMarkerStride / 8, hard, ns8 / 8);
        const int deint_off = meteor_deinterleave_expected_sync_offset(ring_branch_);
        sync_offs = (sync_offs - deint_off + kInterMarkerIntersamps + 1) % kInterMarkerStride;
        if (sync_offs > kInterMarkerStride / 2)
            sync_offs -= kInterMarkerStride;

        if (sync_offs > 0) {
            if (!read(ctx, dst + num_samples, (size_t)sync_offs))
                return 1;
        } else if (sync_offs < 0) {
            std::memcpy(from_prev_.data(), dst + num_samples + (size_t)sync_offs, (size_t)(-sync_offs));
        }

        const int total_rot = (int)num_samples + sync_offs;
        if (total_rot < 0)
            return 1;
        rotate_soft_satdump(dst, (size_t)total_rot, rotation_, false);
        const int dr = meteor_lrpt_deinterleave_dispatch(
            ring_, file_ring_, dst, dst + (size_t)sync_offs, len, ring_offset_, ring_branch_);
        if (dr != 0)
            return dr;
        stream_carry_ = (sync_offs < 0) ? -sync_offs : 0;
    }

    return 0;
}

}  // namespace meteor_lrpt
