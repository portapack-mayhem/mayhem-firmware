/*
 * BPSK / CCSDS CADU deframer — port of SatDump `deframing::BPSK_CCSDS_Deframer`
 * (`src-core/common/codings/deframing/bpsk_ccsds_deframer.{h,cpp}`, MIT).
 *
 * Unpacked bits (0/1 per byte) in; 1024-byte CADUs with ASM at [0..3] out.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Upstream mapping: `SATDUMP_VENDOR.md`.
 */
#ifndef METEOR_LRPT_METEOR_BPSK_CCSDS_DEFRAMER_HPP
#define METEOR_LRPT_METEOR_BPSK_CCSDS_DEFRAMER_HPP

#include <array>
#include <cstdint>
#include <cstring>

class MeteorBpskCcsdsDeframer {
   public:
    static constexpr int kCaduAsmBits = 32;
    static constexpr int kDefaultCaduBits = 8192;
    static constexpr uint32_t kDefaultSync = 0x1ACFFC1DU;

    /* Hamming distance threshold per SatDump state (mutable in SatDump; fixed here). */
    static constexpr int kStateNosync = 2;
    static constexpr int kStateSyncing = 6;
    static constexpr int kStateSynced = 12;

    explicit MeteorBpskCcsdsDeframer(int cadu_bits = kDefaultCaduBits, uint32_t sync_word = kDefaultSync);

    void reset();

    int get_state() const { return d_state_; }

    /**
     * Feeds unpacked bits (0/1 per byte). **Stateful:** `d_state_`, `shifter_`, and partial CADU
     * assembly persist across calls for continuous SatDump-style streaming.
     */
    int work(const uint8_t* input_bits, int num_bits, uint8_t* output_bytes);

   private:
    const uint32_t cadu_asm_;
    const uint32_t cadu_asm_inv_;
    const int cadu_size_bits_;

    int d_state_{kStateNosync};
    bool in_frame_{false};
    uint32_t shifter_{0};
    bool bit_inversion_{false};
    int bit_of_frame_{0};

    int d_invalid_asm_{0};
    int d_good_asm_{0};

    /* Tail bits after CADU_SIZE use indices up to ~1027 (SatDump used oversized alloc). */
    std::array<uint8_t, 1028> frame_buffer_{};

    static int compare_32(uint32_t v1, uint32_t v2);
    void write_bit(uint8_t b);
    void reset_frame();
};

#endif
