/*
 * Copyright (C) 2026
 *
 * Offline Meteor LRPT FEC: soft int8 block → 1020-byte CADU REC.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef BASEBAND_METEOR_LRPT_METEOR_OFFLINE_FEC_HPP
#define BASEBAND_METEOR_LRPT_METEOR_OFFLINE_FEC_HPP

#include "meteor_bpsk_ccsds_deframer.hpp"
#include "meteor_cc_decoder.hpp"
#include "meteor_nrzm.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace meteor_lrpt {

/** PortaPack PM flags (no live interleaved bit1). */
constexpr uint8_t kPmFlagM2x = 1u << 0;
constexpr uint8_t kPmFlagDiff = 1u << 2;
constexpr uint8_t kPmFlagLegacyCorr = 1u << 3;

/** Convert persisted PM flags to M4/baseband FEC flags (bit1 always clear). */
uint8_t fec_flags_from_pm(uint8_t pm_flags);

class OfflineFecDecoder {
   public:
    static constexpr size_t kSoftBlock16384 = 16384;
    static constexpr size_t kSoftBlock8192 = 8192;
    static constexpr size_t kCaduRecBytes = 1020;

    explicit OfflineFecDecoder(uint8_t pm_flags);

    /** Expected soft block size for current flags. */
    size_t soft_block_bytes() const;

    /**
     * Decode one soft block. On success fills `cadu_out` (1020 B Mayhem REC body).
     * @return true if ASM+RS checks passed.
     */
    bool decode_soft_block(const int8_t* soft, size_t nbytes, std::array<uint8_t, kCaduRecBytes>& cadu_out);

   private:
    bool post_fec_frame_checks(std::array<uint8_t, 1024>& frame);
    bool process_soft_to_frame(const int8_t* soft, std::array<uint8_t, 1024>& frame);
    bool process_m2x_noninterleaved(const int8_t* soft, bool diff, std::array<uint8_t, 1024>& frame);
    static void soft_circular_shift(const int8_t* src, size_t len, size_t shift, int8_t* dst);

    uint8_t flags_{0};
    std::array<int8_t, kSoftBlock16384> soft_rot_work_{};
    MeteorCcDecoder viterbi_{8192};
    MeteorNrzmByteDecoder nrzm_{};
    MeteorNrzmBitDecoder nrzm_bits_{};
    MeteorBpskCcsdsDeframer bpsk_deframer_{};
    std::array<uint8_t, 8192 / 8> m2x_bits_{};
    std::array<uint8_t, 2048> deframer_batch_out_{};
    std::array<uint8_t, 1024> frame_work_{};
    std::array<uint8_t, 1024> vb_work_{};
};

}  // namespace meteor_lrpt

#endif /* BASEBAND_METEOR_LRPT_METEOR_OFFLINE_FEC_HPP */
