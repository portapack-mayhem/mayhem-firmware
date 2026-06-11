/*
 * Copyright (C) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "meteor_offline_fec.hpp"

#include "ccsds_derandomize.hpp"
#include "meteor_bpsk_ccsds_deframer.hpp"
#include "meteor_cc_decoder.hpp"
#include "meteor_nrzm.hpp"
#include "meteor_soft_correlate.hpp"
#include "rs223_decode.hpp"

#include <algorithm>
#include <cstring>

namespace meteor_lrpt {

namespace {
constexpr uint8_t kCaduAsm[] = {0x1A, 0xCF, 0xFC, 0x1D};
}  // namespace

uint8_t fec_flags_from_pm(uint8_t pm_flags) {
    pm_flags = static_cast<uint8_t>(pm_flags & ~0x02u);
    uint8_t f = 0;
    if (pm_flags & kPmFlagM2x)
        f |= 1u;
    if (pm_flags & kPmFlagDiff)
        f |= 1u << 2;
    if (pm_flags & kPmFlagLegacyCorr)
        f |= 1u << 3;
    return f;
}

OfflineFecDecoder::OfflineFecDecoder(uint8_t pm_flags)
    : flags_{fec_flags_from_pm(pm_flags)} {}

size_t OfflineFecDecoder::soft_block_bytes() const {
    return (flags_ & 1u) ? kSoftBlock8192 : kSoftBlock16384;
}

void OfflineFecDecoder::soft_circular_shift(const int8_t* src, const size_t len, const size_t shift, int8_t* dst) {
    if (!len)
        return;
    const size_t s = shift % len;
    for (size_t i = 0; i < len; i++)
        dst[i] = src[(i + s) % len];
}

bool OfflineFecDecoder::post_fec_frame_checks(std::array<uint8_t, 1024>& frame) {
    meteor_derand_ccsds(&frame[4], 1020);

    if (frame[9] == 0xff) {
        for (size_t i = 0; i < frame.size(); i++)
            frame[i] ^= 0xff;
    }

    return std::memcmp(&frame[4], kCaduAsm, sizeof(kCaduAsm)) == 0;
}

bool OfflineFecDecoder::process_soft_to_frame(const int8_t* soft, std::array<uint8_t, 1024>& frame) {
    nrzm_.reset();
    viterbi_.decode_soft(soft, frame.data(), frame.size());

    if (flags_ & (1u << 2))
        nrzm_.decode(frame.data(), static_cast<int>(frame.size()));

    return post_fec_frame_checks(frame);
}

bool OfflineFecDecoder::process_m2x_noninterleaved(const int8_t* soft, const bool diff, std::array<uint8_t, 1024>& frame) {
    viterbi_.decode_soft(soft, vb_work_.data(), vb_work_.size());

    size_t bi = 0;
    for (size_t i = 0; i < vb_work_.size(); i++) {
        const uint8_t b = vb_work_[i];
        for (int k = 7; k >= 0; k--)
            m2x_bits_[bi++] = static_cast<uint8_t>((b >> static_cast<unsigned>(k)) & 1u);
    }

    if (diff)
        nrzm_bits_.decode_bits(m2x_bits_.data(), 8192);

    const int n = bpsk_deframer_.work(m2x_bits_.data(), 8192, deframer_batch_out_.data());
    const int max_frames = static_cast<int>(deframer_batch_out_.size() / 1024);
    for (int fi = 0; fi < n && fi < max_frames; fi++) {
        std::memcpy(frame.data(), deframer_batch_out_.data() + static_cast<size_t>(fi) * 1024, 1024);
        if (post_fec_frame_checks(frame))
            return true;
    }
    return false;
}

bool OfflineFecDecoder::decode_soft_block(
    const int8_t* soft,
    const size_t nbytes,
    std::array<uint8_t, kCaduRecBytes>& cadu_out) {
    if (!soft)
        return false;
    const size_t need = soft_block_bytes();
    if (nbytes < need)
        return false;

    const bool m2x = (flags_ & 1u) != 0;
    const bool diff = (flags_ & (1u << 2)) != 0;
    const bool leg_corr = (flags_ & (1u << 3)) != 0;

    bool asm_ok = false;

    if (!m2x) {
        std::memcpy(soft_rot_work_.data(), soft, kSoftBlock16384);
        meteor_lrpt::CorrQpskResult corr_out{};
        meteor_lrpt::correlate_rotate_qpsk_legacy(soft_rot_work_.data(), kSoftBlock16384, diff, &corr_out);
        asm_ok = process_soft_to_frame(soft_rot_work_.data(), frame_work_);
        if (!asm_ok && leg_corr) {
            for (unsigned sh = 2; sh < 64; sh += 2) {
                soft_circular_shift(soft, kSoftBlock16384, sh, soft_rot_work_.data());
                meteor_lrpt::correlate_rotate_qpsk_legacy(soft_rot_work_.data(), kSoftBlock16384, diff, &corr_out);
                if (process_soft_to_frame(soft_rot_work_.data(), frame_work_)) {
                    asm_ok = true;
                    break;
                }
            }
        }
    } else {
        asm_ok = process_m2x_noninterleaved(soft, diff, frame_work_);
    }

    if (!asm_ok)
        return false;

    int16_t rs_e[4]{-1, -1, -1, -1};
    std::memcpy(vb_work_.data(), &frame_work_[4], kCaduRecBytes);
    if (!meteor_lrpt_rs_decode_interleaved_depth4(&frame_work_[4], rs_e)) {
        std::memcpy(&frame_work_[4], vb_work_.data(), kCaduRecBytes);
        return false;
    }

    std::memcpy(cadu_out.data(), &frame_work_[4], kCaduRecBytes);
    return true;
}

}  // namespace meteor_lrpt
