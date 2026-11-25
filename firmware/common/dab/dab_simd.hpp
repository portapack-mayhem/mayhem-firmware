/*
 * Copyright (C) 2026 Mayhem firmware contributors
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

/* Single-instruction SIMD32 helpers for the transforms and the OFDM front end;
 * this toolchain's arm_acle.h lacks the intrinsics. One instruction per
 * template keeps an output landing on a live input harmless ("=&r" otherwise).
 * DAB_SIMD_PORTABLE swaps in exact plain-C twins for a host; the includer sets
 * it from DAB_FFT_SIMD_PORTABLE or DAB_DEMAP_SIMD_PORTABLE. */

#ifndef __DAB_SIMD_H__
#define __DAB_SIMD_H__

#include <cstdint>

#if defined(DAB_SIMD_PORTABLE)
/* 16-bit lanes, real in the low half; multiplies accumulate modulo 2^32, adds
 * saturate or halve per lane. QSAX and QASX were checked lane by lane against
 * the ARM ARM pseudocode. */
static inline int16_t dsp_lane_lo(uint32_t v) { return static_cast<int16_t>(v & 0xFFFFu); }
static inline int16_t dsp_lane_hi(uint32_t v) { return static_cast<int16_t>(v >> 16); }
static inline uint32_t dsp_lane_pack(int32_t lo, int32_t hi) {
    return (static_cast<uint32_t>(static_cast<uint16_t>(hi)) << 16) |
           static_cast<uint32_t>(static_cast<uint16_t>(lo));
}
static inline int16_t dsp_lane_sat(int32_t v) {
    if (v > 32767) return 32767;
    if (v < -32768) return -32768;
    return static_cast<int16_t>(v);
}

/* SMUSD: Rn[15:0]*Rm[15:0] - Rn[31:16]*Rm[31:16] */
static inline int32_t dsp_smusd(uint32_t a, uint32_t b) {
    const uint32_t lo = static_cast<uint32_t>(static_cast<int32_t>(dsp_lane_lo(a)) * dsp_lane_lo(b));
    const uint32_t hi = static_cast<uint32_t>(static_cast<int32_t>(dsp_lane_hi(a)) * dsp_lane_hi(b));
    return static_cast<int32_t>(lo - hi);
}

/* SMUADX: Rn[15:0]*Rm[31:16] + Rn[31:16]*Rm[15:0] */
static inline int32_t dsp_smuadx(uint32_t a, uint32_t b) {
    const uint32_t x = static_cast<uint32_t>(static_cast<int32_t>(dsp_lane_lo(a)) * dsp_lane_hi(b));
    const uint32_t y = static_cast<uint32_t>(static_cast<int32_t>(dsp_lane_hi(a)) * dsp_lane_lo(b));
    return static_cast<int32_t>(x + y);
}

/* SMUAD: Rn[15:0]*Rm[15:0] + Rn[31:16]*Rm[31:16] */
static inline int32_t dsp_smuad(uint32_t a, uint32_t b) {
    const uint32_t lo = static_cast<uint32_t>(static_cast<int32_t>(dsp_lane_lo(a)) * dsp_lane_lo(b));
    const uint32_t hi = static_cast<uint32_t>(static_cast<int32_t>(dsp_lane_hi(a)) * dsp_lane_hi(b));
    return static_cast<int32_t>(lo + hi);
}

/* SMUSDX: Rn[15:0]*Rm[31:16] - Rn[31:16]*Rm[15:0] */
static inline int32_t dsp_smusdx(uint32_t a, uint32_t b) {
    const uint32_t x = static_cast<uint32_t>(static_cast<int32_t>(dsp_lane_lo(a)) * dsp_lane_hi(b));
    const uint32_t y = static_cast<uint32_t>(static_cast<int32_t>(dsp_lane_hi(a)) * dsp_lane_lo(b));
    return static_cast<int32_t>(x - y);
}

/* PKHBT Rd, Rn, Rm, LSL #1: Rd[31:16] = Rm[30:15], the low half of Rm ASR #15. */
static inline uint32_t dsp_pack_lsl1(uint32_t lo_shifted, uint32_t hi_raw) {
    return ((hi_raw << 1) & 0xFFFF0000u) | (lo_shifted & 0xFFFFu);
}

static inline uint32_t dsp_qadd16(uint32_t a, uint32_t b) {
    return dsp_lane_pack(dsp_lane_sat(int32_t(dsp_lane_lo(a)) + dsp_lane_lo(b)),
                         dsp_lane_sat(int32_t(dsp_lane_hi(a)) + dsp_lane_hi(b)));
}

static inline uint32_t dsp_qsub16(uint32_t a, uint32_t b) {
    return dsp_lane_pack(dsp_lane_sat(int32_t(dsp_lane_lo(a)) - dsp_lane_lo(b)),
                         dsp_lane_sat(int32_t(dsp_lane_hi(a)) - dsp_lane_hi(b)));
}

static inline uint32_t dsp_shadd16(uint32_t a, uint32_t b) {
    return dsp_lane_pack((int32_t(dsp_lane_lo(a)) + dsp_lane_lo(b)) >> 1,
                         (int32_t(dsp_lane_hi(a)) + dsp_lane_hi(b)) >> 1);
}

static inline uint32_t dsp_shsub16(uint32_t a, uint32_t b) {
    return dsp_lane_pack((int32_t(dsp_lane_lo(a)) - dsp_lane_lo(b)) >> 1,
                         (int32_t(dsp_lane_hi(a)) - dsp_lane_hi(b)) >> 1);
}

/* QSAX: Rd[15:0] = sat(Rn[15:0] + Rm[31:16]), Rd[31:16] = sat(Rn[31:16] - Rm[15:0]).
 * QASX exchanges the two signs. */
static inline uint32_t dsp_qsax(uint32_t a, uint32_t b) {
    return dsp_lane_pack(dsp_lane_sat(int32_t(dsp_lane_lo(a)) + dsp_lane_hi(b)),
                         dsp_lane_sat(int32_t(dsp_lane_hi(a)) - dsp_lane_lo(b)));
}

static inline uint32_t dsp_qasx(uint32_t a, uint32_t b) {
    return dsp_lane_pack(dsp_lane_sat(int32_t(dsp_lane_lo(a)) - dsp_lane_hi(b)),
                         dsp_lane_sat(int32_t(dsp_lane_hi(a)) + dsp_lane_lo(b)));
}
#elif defined(__ARM_FEATURE_SIMD32)
#define DAB_SIMD_OP(type, insn)                                \
    static inline type dsp_##insn(uint32_t a, uint32_t b) {    \
        type r;                                                \
        __asm(#insn " %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); \
        return r;                                              \
    }
DAB_SIMD_OP(int32_t, smusd)
DAB_SIMD_OP(int32_t, smuadx)
DAB_SIMD_OP(int32_t, smuad)
DAB_SIMD_OP(int32_t, smusdx)
DAB_SIMD_OP(uint32_t, qadd16)
DAB_SIMD_OP(uint32_t, qsub16)
DAB_SIMD_OP(uint32_t, shadd16)
DAB_SIMD_OP(uint32_t, shsub16)
DAB_SIMD_OP(uint32_t, qsax)
DAB_SIMD_OP(uint32_t, qasx)
#undef DAB_SIMD_OP

/* lo_shifted in the bottom halfword, hi_raw << 1 in the top. */
static inline uint32_t dsp_pack_lsl1(uint32_t lo_shifted, uint32_t hi_raw) {
    uint32_t r;
    __asm("pkhbt %0, %1, %2, lsl #1" : "=r"(r) : "r"(lo_shifted), "r"(hi_raw));
    return r;
}
#endif

static inline int16_t saturate16(int32_t v) {
    if (v > 32767)
        return 32767;
    if (v < -32768)
        return -32768;
    return static_cast<int16_t>(v);
}

static inline __attribute__((always_inline)) uint32_t reverse_bits32(uint32_t v) {
#if defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7A__)
    uint32_t r;
    __asm("rbit %0, %1" : "=r"(r) : "r"(v));
    return r;
#else
    v = ((v & 0x55555555u) << 1) | ((v >> 1) & 0x55555555u);
    v = ((v & 0x33333333u) << 2) | ((v >> 2) & 0x33333333u);
    v = ((v & 0x0F0F0F0Fu) << 4) | ((v >> 4) & 0x0F0F0F0Fu);
    return __builtin_bswap32(v);
#endif
}

#endif /*__DAB_SIMD_H__*/
