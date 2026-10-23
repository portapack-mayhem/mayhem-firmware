// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DSP_REPLAY_INTERPOLATE_HPP
#define DSP_REPLAY_INTERPOLATE_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#if defined(LPC43XX_M4)
#include "simd.hpp"
#endif

namespace dsp {
namespace replay {

// Frozen Q15 coefficients for Replay HB47/HB11 interpolation.
// Unique symmetric Q15 pairs, nearest center first. Odd-indexed taps are zero
// except the center, a delayed copy with implicit Q15 coefficient 32768.
// Both interpolation phases have unity DC gain: the filtered branch sums to
// 32768 and the delayed-copy phase has gain 32768. The prototype sums to 65536.
// Response figures below describe individual FIR prototypes, before repetition.
// HB47: 24 stored-branch nonzero taps, 12 unique pairs.
// Edges / stage output Fs: pass=0.2125, stop=0.2875.
// Q15 ripple=0.012143 dB; rejection=63.001 dB.
constexpr std::array<int16_t, 12> hb47_pairs{{20774, -6692, 3753, -2413, 1632, -1110, 751, -490, 311, -180, 100, -52}};
// HB11: 6 stored-branch nonzero taps, 3 unique pairs.
// Edges / stage output Fs: pass=0.1125, stop=0.3875.
// Q15 ripple=0.016364 dB; rejection=60.430 dB.
constexpr std::array<int16_t, 3> hb11_pairs{{19639, -3954, 699}};

// Signed int32 components with four fractional bits beyond C16.
// With normalized C16 input, unity is 2^19. Preserve intermediate overshoot;
// saturate only when producing final C8 samples.
struct Sample {
    int32_t i{0};
    int32_t q{0};
};

inline int32_t round_shift(int64_t value, unsigned bits) {
    const int64_t half = int64_t{1} << (bits - 1);
    // Arithmetic right shift on the supported ARM/GCC and host targets.
    // Bounds below keep the bias addition inside int64, including overshoot.
    const uint32_t negative = uint32_t(uint64_t(value) >> 63);
    // Make the nonnegative bias explicit; GCC 9 otherwise normalizes it with
    // a second 64-bit subtract/carry sequence in every filtered component.
    if (bits <= 32) {
        const uint32_t bias = uint32_t(half) - negative;
        return int32_t((value + int64_t(bias)) >> bits);
    }
    return int32_t((value + half - negative) >> bits);
}

inline int8_t saturate_c8(int32_t rounded, uint32_t* clipped = nullptr) {
#if defined(LPC43XX_M4)
    const int32_t saturated = int32_t(__SSAT(rounded, 8));
    if (clipped && saturated != rounded) ++*clipped;
    return int8_t(saturated);
#else
    if (rounded > 127 || rounded < -128) {
        if (clipped) ++*clipped;
        return rounded > 127 ? 127 : -128;
    }
    return int8_t(rounded);
#endif
}

inline int8_t to_c8(int32_t value, uint32_t* clipped = nullptr) {
    // The six-stage L1 bound is <8.8e6; addition and pair sums fit int32.
    const auto rounded = (value + 2048 - (value < 0)) >> 12;
    return saturate_c8(rounded, clipped);
}

inline int32_t round_filtered_c8(int64_t value) {
    // Preserve two rounds to nearest, ties away from zero: first by 15 bits,
    // then by 12 bits. This is not a single 27-bit rounding.
    // Negative bias differs by 32769; all accumulators fit 40 signed bits.
    const uint32_t bias = 67125248u - uint32_t(uint64_t(value) >> 63) * 32769u;
    return int32_t((value + int64_t(bias)) >> 27);
}

inline int64_t hb11_component(int32_t v0, int32_t v1, int32_t v2, int32_t v3, int32_t v4, int32_t v5) {
    return int64_t(v2 + v3) * hb11_pairs[0] +
           int64_t(v1 + v4) * hb11_pairs[1] +
           int64_t(v0 + v5) * hb11_pairs[2];
}

template <bool Imag>
inline int32_t lane(const Sample& value) {
    return Imag ? value.q : value.i;
}

// Fixed offsets and coefficients; one accumulator per component keeps the
// twelve-pair first stage independent of compiler scheduling/register pressure.
template <bool Imag>
inline int64_t hb47_component(const Sample* w) {
    int64_t a = int64_t(lane<Imag>(w[11]) + lane<Imag>(w[12])) * hb47_pairs[0];
    a += int64_t(lane<Imag>(w[10]) + lane<Imag>(w[13])) * hb47_pairs[1];
    a += int64_t(lane<Imag>(w[9]) + lane<Imag>(w[14])) * hb47_pairs[2];
    a += int64_t(lane<Imag>(w[8]) + lane<Imag>(w[15])) * hb47_pairs[3];
    a += int64_t(lane<Imag>(w[7]) + lane<Imag>(w[16])) * hb47_pairs[4];
    a += int64_t(lane<Imag>(w[6]) + lane<Imag>(w[17])) * hb47_pairs[5];
    a += int64_t(lane<Imag>(w[5]) + lane<Imag>(w[18])) * hb47_pairs[6];
    a += int64_t(lane<Imag>(w[4]) + lane<Imag>(w[19])) * hb47_pairs[7];
    a += int64_t(lane<Imag>(w[3]) + lane<Imag>(w[20])) * hb47_pairs[8];
    a += int64_t(lane<Imag>(w[2]) + lane<Imag>(w[21])) * hb47_pairs[9];
    a += int64_t(lane<Imag>(w[1]) + lane<Imag>(w[22])) * hb47_pairs[10];
    a += int64_t(lane<Imag>(w[0]) + lane<Imag>(w[23])) * hb47_pairs[11];
    return a;
}

__attribute__((noinline)) inline void hb47_block(const Sample* window, size_t count, Sample* output) {
    for (size_t n = 0; n < count; ++n) {
        output[2 * n].i = round_shift(hb47_component<false>(window + n), 15);
        output[2 * n + 1].i = window[n + 12].i;
    }
    for (size_t n = 0; n < count; ++n) {
        output[2 * n].q = round_shift(hb47_component<true>(window + n), 15);
        output[2 * n + 1].q = window[n + 12].q;
    }
}

template <bool Final, bool Imag, typename Output>
inline void emit_halfband(int64_t filtered, int32_t delayed, Output* output, uint32_t* clipped) {
    if constexpr (Final) {
        const auto even = saturate_c8(round_filtered_c8(filtered), clipped);
        const auto odd = to_c8(delayed, clipped);
        output[Imag ? 1 : 0] = uint8_t(even);
        output[Imag ? 3 : 2] = uint8_t(odd);
    } else {
        (void)clipped;
        if constexpr (Imag) {
            output[0].q = round_shift(filtered, 15);
            output[1].q = delayed;
        } else {
            output[0].i = round_shift(filtered, 15);
            output[1].i = delayed;
        }
    }
}

// Separate I/Q passes reduce register pressure. HB11 input counts are even
// because the preceding stage doubles its input count.
template <bool Final, bool Imag, typename Output>
__attribute__((always_inline)) inline void hb11_lane(const Sample* window, size_t count, Output* output, uint32_t* clipped) {
    int32_t v0 = lane<Imag>(window[0]);
    int32_t v1 = lane<Imag>(window[1]);
    int32_t v2 = lane<Imag>(window[2]);
    int32_t v3 = lane<Imag>(window[3]);
    int32_t v4 = lane<Imag>(window[4]);
    size_t n = 0;
    // Six phases rotate the logical window without moving its register values.
    // Process the remainder two input samples at a time.
    for (; n + 6 <= count; n += 6) {
        const int32_t v5 = lane<Imag>(window[n + 5]);
        emit_halfband<Final, Imag>(hb11_component(v0, v1, v2, v3, v4, v5), v3, output + (Final ? 4 : 2) * n, clipped);
        v0 = lane<Imag>(window[n + 6]);
        emit_halfband<Final, Imag>(hb11_component(v1, v2, v3, v4, v5, v0), v4, output + (Final ? 4 : 2) * (n + 1), clipped);
        v1 = lane<Imag>(window[n + 7]);
        emit_halfband<Final, Imag>(hb11_component(v2, v3, v4, v5, v0, v1), v5, output + (Final ? 4 : 2) * (n + 2), clipped);
        v2 = lane<Imag>(window[n + 8]);
        emit_halfband<Final, Imag>(hb11_component(v3, v4, v5, v0, v1, v2), v0, output + (Final ? 4 : 2) * (n + 3), clipped);
        v3 = lane<Imag>(window[n + 9]);
        emit_halfband<Final, Imag>(hb11_component(v4, v5, v0, v1, v2, v3), v1, output + (Final ? 4 : 2) * (n + 4), clipped);
        v4 = lane<Imag>(window[n + 10]);
        emit_halfband<Final, Imag>(hb11_component(v5, v0, v1, v2, v3, v4), v2, output + (Final ? 4 : 2) * (n + 5), clipped);
    }
    for (; n < count; n += 2) {
        const int32_t v5 = lane<Imag>(window[n + 5]);
        emit_halfband<Final, Imag>(hb11_component(v0, v1, v2, v3, v4, v5), v3, output + (Final ? 4 : 2) * n, clipped);
        const int32_t v6 = lane<Imag>(window[n + 6]);
        emit_halfband<Final, Imag>(hb11_component(v1, v2, v3, v4, v5, v6), v4, output + (Final ? 4 : 2) * (n + 1), clipped);
        v0 = v2;
        v1 = v3;
        v2 = v4;
        v3 = v5;
        v4 = v6;
    }
}

// Block boundaries are deliberately out of line: compiler spills must not
// compete with the hot FIR register window. Calls occur only once per tile.
__attribute__((noinline)) inline void hb11_wide(const Sample* window, size_t count, Sample* output) {
    hb11_lane<false, false>(window, count, output, nullptr);
    hb11_lane<false, true>(window, count, output, nullptr);
}

__attribute__((noinline)) inline void hb11_final(const Sample* window, size_t count, unsigned char* output, uint32_t* clipped) {
    uint32_t block_clipped = 0;
    hb11_lane<true, false>(window, count, output, &block_clipped);
    hb11_lane<true, true>(window, count, output, &block_clipped);
    if (clipped) *clipped += block_clipped;
}

// Quantize once, then emit the already packed C8 value. memcpy keeps stores
// alias-safe; GCC emits halfword/word stores on the supported little-endian
// targets. Clipping counts describe physical output components, including R.
template <typename Output>
inline void emit_repeated(int8_t i, int8_t q, size_t repeat, Output* output) {
    static_assert(std::is_trivially_copyable<Output>::value, "Replay samples must be trivially copyable");
    static_assert(sizeof(Output) == 2, "Replay hardware samples must be complex C8");
    const uint16_t pair = uint16_t(uint8_t(i)) | (uint16_t(uint8_t(q)) << 8);
    if (repeat == 1) {
        memcpy(static_cast<void*>(output), &pair, sizeof(pair));
    } else {
        const uint32_t word = uint32_t(pair) | (uint32_t(pair) << 16);
        for (size_t n = 0; n < repeat; n += 2) memcpy(static_cast<void*>(output + n), &word, sizeof(word));
    }
}

template <typename Output>
__attribute__((noinline)) void repeat_c8(const unsigned char* input, size_t count, size_t repeat, Output* output) {
    if (repeat == 2) {
        size_t n = 0;
        for (; n + 1 < count; n += 2) {
            uint32_t pairs;
            memcpy(&pairs, input + 2 * n, sizeof(pairs));
#if defined(LPC43XX_M4)
            // Bit packing of C8 IQ pairs, not arithmetic on narrowed Q19.
            const uint32_t first = __PKHBT(pairs, pairs, 16);
            const uint32_t second = __PKHTB(pairs, pairs, 16);
#else
            const uint32_t first = (pairs & 65535u) | (pairs << 16);
            const uint32_t second = (pairs >> 16) | (pairs & 0xffff0000u);
#endif
            memcpy(static_cast<void*>(output + 2 * n), &first, sizeof(first));
            memcpy(static_cast<void*>(output + 2 * n + 2), &second, sizeof(second));
        }
        if (n < count) emit_repeated(int8_t(input[2 * n]), int8_t(input[2 * n + 1]), 2, output + 2 * n);
    } else {
        for (size_t n = 0; n < count; ++n)
            emit_repeated(int8_t(input[2 * n]), int8_t(input[2 * n + 1]), repeat, output + n * repeat);
    }
}

__attribute__((noinline)) inline void hb47_final(const Sample* window, size_t count, unsigned char* output, uint32_t* clipped) {
    uint32_t hits = 0;
    for (size_t n = 0; n < count; ++n)
        emit_halfband<true, false>(hb47_component<false>(window + n), window[n + 12].i, output + 4 * n, &hits);
    for (size_t n = 0; n < count; ++n)
        emit_halfband<true, true>(hb47_component<true>(window + n), window[n + 12].q, output + 4 * n, &hits);
    if (clipped) *clipped += hits;
}

template <typename Input, typename Output>
__attribute__((noinline)) void direct_c16(const Input* input, size_t count, Output* output, size_t output_count, uint32_t* clipped) {
    uint32_t hits = 0;
    auto* bytes = reinterpret_cast<unsigned char*>(output);
    for (size_t n = 0; n < count; ++n) {
        const int32_t i = input[n].real();
        const int32_t q = input[n].imag();
        bytes[2 * n] = uint8_t(saturate_c8((i + 128 - (i < 0)) >> 8, &hits));
        bytes[2 * n + 1] = uint8_t(saturate_c8((q + 128 - (q < 0)) >> 8, &hits));
    }
    for (size_t n = count; n < output_count; ++n) output[n] = {};
    if (clipped) *clipped += hits;
}

// FIR factor M = 2^N: one HB47 stage followed by N-1 HB11 stages.
// All HB11 stages share coefficients but retain independent histories.
// I and Q use identical real filters. Quantize to C8 before repetition R;
// the total output factor is M*R. M=1 bypasses the FIR stages.
// Bounded blocks share two payload buffers. Each stage owns only its history;
// the producer writes new samples after the next consumer's history prefix.
class InterpolationCascade {
   public:
    static constexpr size_t payload_count = 64;
    static constexpr size_t output_tile = 2 * payload_count;

    bool configure(size_t factor, size_t fir_factor = 0) {
        stage_count_ = 0;
        total_factor_ = 0;
        if (factor < 1 || factor > 64 || (factor & (factor - 1))) return false;
        if (!fir_factor) fir_factor = factor;
        if (fir_factor > factor || (fir_factor & (fir_factor - 1))) return false;
        total_factor_ = factor;
        while (fir_factor > 1) {
            ++stage_count_;
            fir_factor >>= 1;
        }
        reset();
        return true;
    }

    void reset() {
        first_window_.fill({});
        for (auto& history : later_history_) history.fill({});
    }

    // Total output factor, including both FIR interpolation and repetition.
    size_t factor() const { return total_factor_; }
    size_t fir_factor() const { return total_factor_ ? size_t{1} << stage_count_ : 0; }
    size_t repetition_factor() const { return total_factor_ >> stage_count_; }

    template <typename Input, typename Output>
    __attribute__((noinline)) void execute_buffer(const Input* input, size_t input_count, Output* output, size_t output_count, uint32_t* clipped = nullptr) {
        const auto interpolation_factor = factor();
        if (!interpolation_factor || output_count % interpolation_factor) {
            for (size_t n = 0; n < output_count; ++n) output[n] = {};
            return;
        }
        const size_t source_count = output_count / interpolation_factor;
        const size_t repeat = repetition_factor();
        if (!stage_count_ && repeat == 1) {
            direct_c16(input, input_count < source_count ? input_count : source_count, output, output_count, clipped);
            return;
        }
        if (!stage_count_) {
            uint32_t hits = 0;
            for (size_t n = 0; n < source_count; ++n) {
                const int32_t i = n < input_count ? input[n].real() : 0;
                const int32_t q = n < input_count ? input[n].imag() : 0;
                const auto ci = saturate_c8((i + 128 - (i < 0)) >> 8, &hits);
                const auto cq = saturate_c8((q + 128 - (q < 0)) >> 8, &hits);
                emit_repeated(ci, cq, repeat, output + n * repeat);
            }
            if (clipped) *clipped += hits * repeat;
            return;
        }
        const size_t tile = output_tile / interpolation_factor;
        const size_t source_tile = tile < 32 ? tile : 32;
        for (size_t offset = 0; offset < source_count;) {
            const size_t remaining = source_count - offset;
            const size_t count = remaining < source_tile ? remaining : source_tile;
            for (size_t n = 0; n < count; ++n) {
                const size_t index = offset + n;
                first_window_[23 + n] = index < input_count
                                            ? Sample{int32_t(input[index].real()) * 16, int32_t(input[index].imag()) * 16}
                                            : Sample{};
            }
            auto* src = scratch_[0].data();
            auto* dst = scratch_[1].data();
            auto* tx = output + offset * interpolation_factor;
            if (stage_count_ == 1) {
                uint32_t hits = 0;
                hb47_final(first_window_.data(), count, repeat == 1 ? reinterpret_cast<unsigned char*>(tx) : filtered_.data(), &hits);
                if (repeat > 1) repeat_c8(filtered_.data(), 2 * count, repeat, tx);
                if (clipped) *clipped += hits * repeat;
                for (size_t n = 0; n < 23; ++n) first_window_[n] = first_window_[count + n];
                offset += count;
                continue;
            }
            hb47_block(first_window_.data(), count, src + 5);
            // Forward copy is safe even when the old/new windows overlap.
            for (size_t n = 0; n < 23; ++n) first_window_[n] = first_window_[count + n];
            size_t wide_count = 2 * count;
            for (size_t stage = 1; stage + 1 < stage_count_; ++stage) {
                prepare_history(src, stage - 1);
                hb11_wide(src, wide_count, dst + 5);
                retain_history(src, wide_count, stage - 1);
                auto* swap = src;
                src = dst;
                dst = swap;
                wide_count *= 2;
            }
            prepare_history(src, stage_count_ - 2);
            if (repeat == 1) {
                hb11_final(src, wide_count, reinterpret_cast<unsigned char*>(tx), clipped);
            } else {
                uint32_t hits = 0;
                hb11_final(src, wide_count, filtered_.data(), &hits);
                repeat_c8(filtered_.data(), 2 * wide_count, repeat, tx);
                if (clipped) *clipped += hits * repeat;
            }
            retain_history(src, wide_count, stage_count_ - 2);
            offset += count;
        }
    }

    // Small host/test callers use the identical block path and histories.
    template <typename Output>
    void execute(int16_t i, int16_t q, Output* output, uint32_t* clipped = nullptr) {
        struct Input {
            int16_t i, q;
            int16_t real() const { return i; }
            int16_t imag() const { return q; }
        };
        const Input input{i, q};
        execute_buffer(&input, 1, output, factor(), clipped);
    }

   private:
    void prepare_history(Sample* window, size_t stage) {
        for (size_t n = 0; n < 5; ++n) window[n] = later_history_[stage][n];
    }
    void retain_history(const Sample* window, size_t count, size_t stage) {
        for (size_t n = 0; n < 5; ++n) later_history_[stage][n] = window[count + n];
    }

    std::array<Sample, 23 + output_tile / 4> first_window_{};
    std::array<std::array<Sample, 5>, 5> later_history_{};
    std::array<std::array<Sample, 5 + payload_count>, 2> scratch_{};
    std::array<unsigned char, 2 * payload_count> filtered_{};
    size_t stage_count_{0};
    size_t total_factor_{0};
};

}  // namespace replay
}  // namespace dsp
#endif
