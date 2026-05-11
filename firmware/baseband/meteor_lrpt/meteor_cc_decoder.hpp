/*
 * K=7 R=1/2 CCSDS-style Viterbi decoder (truncated/streaming path) — adapted from
 * GNU Radio `gr-fec` cc_decoder_impl, VOLK `volk_8u_x4_conv_k7_r2_8u`, and SatDump
 * `src-core/common/codings/viterbi/cc_decoder.cpp` usage (see `SATDUMP_VENDOR.md`).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef METEOR_LRPT_METEOR_CC_DECODER_HPP
#define METEOR_LRPT_METEOR_CC_DECODER_HPP

#include <array>
#include <cstddef>
#include <cstdint>

class MeteorCcDecoder {
   public:
    /* frame_bits: decoded information bit count (e.g. 8192 for 1024-byte LRPT frame). */
    explicit MeteorCcDecoder(unsigned int frame_bits);
    MeteorCcDecoder(const MeteorCcDecoder&) = delete;
    MeteorCcDecoder& operator=(const MeteorCcDecoder&) = delete;

    /* soft_in length must be >= 2 * (frame_bits + 6); values int8 centered at 0. */
    void decode_soft(const int8_t* soft_in, uint8_t* out_bytes, size_t out_len_bytes);

   private:
    using conv_kernel = void (*)(unsigned char* Y,
                                 unsigned char* X,
                                 unsigned char* syms,
                                 unsigned char* dec,
                                 unsigned int framebits,
                                 unsigned int excess,
                                 unsigned char* Branchtab);

    static int parity_int(int x);
    void create_viterbi();
    int init_viterbi_unbiased();
    int init_viterbi(int starting_state);
    int find_endstate();
    int update_viterbi_blk(unsigned char* syms, int nbits);
    int chainback_viterbi(unsigned char* data,
                          unsigned int nbits,
                          unsigned int endstate,
                          unsigned int tailsize);

    static constexpr unsigned int d_k = 7;
    static constexpr unsigned int d_rate = 2;
    static constexpr unsigned int d_numstates = 1U << (d_k - 1);
    static constexpr int d_decision_t_size = (int)(d_numstates / 8);

    unsigned int d_frame_size{0};
    unsigned int d_veclen{0};
    int d_ADDSHIFT{0};
    int d_SUBSHIFT{0};

    conv_kernel d_kernel{nullptr};
    std::array<unsigned char, d_numstates / 2 * d_rate> d_branchtab{};

    unsigned char* old_metrics{nullptr};
    unsigned char* new_metrics{nullptr};

    static constexpr size_t max_veclen = 8200;
    static constexpr size_t max_decisions = max_veclen * 8;
    std::array<unsigned char, max_decisions> decisions{};

    int d_start_state_chaining{0};
    int d_end_state_chaining{0};

    std::array<int, 2> d_polys{{79, 109}};
};

#endif
