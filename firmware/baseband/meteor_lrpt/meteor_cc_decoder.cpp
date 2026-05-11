/*
 * MeteorCcDecoder — CCSDS K=7 R=1/2 Viterbi (streaming) using GNU Radio VOLK generic BFLY
 * (`volk_k7_r2_generic_fixed.h`) and chainback/update from GNU Radio `cc_decoder_impl`.
 * SatDump `CCDecoder` constants/polynomials match this path (see `SATDUMP_VENDOR.md`).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "meteor_cc_decoder.hpp"

#include "volk_k7_r2_generic_fixed.h"

#include <algorithm>
#include <cstring>

namespace {

struct Vp {
    uint8_t metrics1[64]{};
    uint8_t metrics2[64]{};
};

}  // namespace

int MeteorCcDecoder::parity_int(int x) {
    unsigned v = (unsigned)x;
    v ^= v >> 16;
    v ^= v >> 8;
    v ^= v >> 4;
    v ^= v >> 2;
    v ^= v >> 1;
    return (int)(v & 1U);
}

void MeteorCcDecoder::create_viterbi() {
    for (unsigned int state = 0; state < d_numstates / 2; state++) {
        for (unsigned int i = 0; i < d_rate; i++) {
            const int poly = d_polys[i];
            const int pabs = poly < 0 ? -poly : poly;
            const int par = parity_int((2 * (int)state) & pabs);
            const int inv = (poly < 0) ? 1 : 0;
            d_branchtab[i * d_numstates / 2 + state] = (unsigned char)((inv ^ par) ? 255 : 0);
        }
    }
}

MeteorCcDecoder::MeteorCcDecoder(unsigned int frame_bits)
    : d_frame_size{frame_bits} {
    d_veclen = d_frame_size + d_k - 1;
    if (d_k - 1U < 8U) {
        d_ADDSHIFT = (int)(8 - (d_k - 1));
        d_SUBSHIFT = 0;
    } else if (d_k - 1U > 8U) {
        d_ADDSHIFT = 0;
        d_SUBSHIFT = (int)((d_k - 1) - 8);
    } else {
        d_ADDSHIFT = 0;
        d_SUBSHIFT = 0;
    }
    d_kernel = volk_fixed::volk_8u_x4_conv_k7_r2_8u_generic;
    create_viterbi();
}

int MeteorCcDecoder::init_viterbi_unbiased() {
    for (unsigned i = 0; i < d_numstates; i++) {
        old_metrics[i] = 31;
        new_metrics[i] = 31;
    }
    return 0;
}

int MeteorCcDecoder::init_viterbi(int starting_state) {
    for (unsigned i = 0; i < d_numstates; i++) {
        old_metrics[i] = 63;
        new_metrics[i] = 63;
    }
    old_metrics[starting_state & (d_numstates - 1)] = 0;
    return 0;
}

int MeteorCcDecoder::find_endstate() {
    unsigned char* met = (((d_k + d_veclen) % 2U) == 0U) ? new_metrics : old_metrics;
    unsigned char minv = met[0];
    int state = 0;
    for (unsigned i = 1; i < d_numstates; ++i) {
        if (met[i] < minv) {
            minv = met[i];
            state = (int)i;
        }
    }
    return state;
}

int MeteorCcDecoder::update_viterbi_blk(unsigned char* syms, int nbits) {
    (void)memset(decisions.data(), 0, (size_t)d_decision_t_size * (size_t)nbits);
    d_kernel(new_metrics,
             old_metrics,
             syms,
             decisions.data(),
             (unsigned int)(nbits - (int)(d_k - 1)),
             d_k - 1,
             d_branchtab.data());
    return 0;
}

int MeteorCcDecoder::chainback_viterbi(unsigned char* data,
                                       unsigned int nbits,
                                       unsigned int endstate,
                                       unsigned int tailsize) {
    unsigned char* d = decisions.data();
    endstate = (endstate % d_numstates) << d_ADDSHIFT;
    d += tailsize * d_decision_t_size;
    int retval = 0;
    int dif = (int)tailsize - (int)(d_k - 1);
    while (nbits-- > d_frame_size - (d_k - 1)) {
        const size_t row = (size_t)nbits * (unsigned)d_decision_t_size;
        const auto es = (unsigned)(endstate >> d_ADDSHIFT);
        const int k = (int)((d[row + es / 8U] >> (es % 8U)) & 1U);
        endstate = (endstate >> 1) | ((unsigned)k << (d_k - 2 + (unsigned)d_ADDSHIFT));
        data[((nbits + (unsigned)dif) % d_frame_size)] = (unsigned char)k;
        retval = (int)endstate;
    }
    nbits += 1;
    while (nbits-- != 0) {
        const size_t row = (size_t)nbits * (unsigned)d_decision_t_size;
        const auto es = (unsigned)(endstate >> d_ADDSHIFT);
        const int k = (int)((d[row + es / 8U] >> (es % 8U)) & 1U);
        endstate = (endstate >> 1) | ((unsigned)k << (d_k - 2 + (unsigned)d_ADDSHIFT));
        data[((nbits + (unsigned)dif) % d_frame_size)] = (unsigned char)k;
    }
    return retval >> d_ADDSHIFT;
}

void MeteorCcDecoder::decode_soft(const int8_t* soft_in, uint8_t* out_bytes, size_t out_len_bytes) {
    static Vp vp{};
    old_metrics = vp.metrics1;
    new_metrics = vp.metrics2;

    const size_t need_syms = (size_t)d_veclen * 2U;
    std::array<unsigned char, max_veclen * 2> syms{};
    constexpr size_t soft_in_cap = 16384;
    for (size_t i = 0; i < need_syms; i++) {
        const int8_t v = (i < soft_in_cap) ? soft_in[i] : 0;
        syms[i] = (unsigned char)((int)v + 128);
    }

    init_viterbi_unbiased();
    update_viterbi_blk(syms.data(), (int)d_veclen);
    d_end_state_chaining = find_endstate();

    std::array<unsigned char, 9000> bit_out{};
    d_start_state_chaining = chainback_viterbi(
        bit_out.data(),
        d_frame_size,
        (unsigned)d_end_state_chaining,
        d_veclen - d_frame_size);

    init_viterbi(d_start_state_chaining);

    const size_t nbytes = std::min(out_len_bytes, d_frame_size / 8U);
    for (size_t b = 0; b < nbytes; b++) {
        uint8_t v = 0;
        for (int j = 0; j < 8; j++)
            v = (uint8_t)((v << 1) | (bit_out[b * 8U + (size_t)j] & 1U));
        out_bytes[b] = v;
    }
}
