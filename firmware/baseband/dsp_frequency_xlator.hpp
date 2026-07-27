/*
 * Copyright (C) 2026
 *
 * This file is part of PortaPack.
 */

#ifndef __DSP_FREQUENCY_XLATOR_H__
#define __DSP_FREQUENCY_XLATOR_H__

#include "dsp_decimate.hpp"
#include "dsp_types.hpp"

#include <array>
#include <cstdint>

namespace dsp {

/* Fixed-point complex mixer for the Audio RX channelizer. */
class FrequencyTranslator {
   public:
    FrequencyTranslator() {
        for (size_t i = 0; i < oscillator_q15_.size(); ++i) {
            oscillator_q15_[i] =
                static_cast<uint16_t>(sine_q15_[static_cast<uint8_t>(i + 64)]) |
                (static_cast<uint32_t>(
                     static_cast<uint16_t>(sine_q15_[i]))
                 << 16);
        }
    }

    void set_sample_rate(const uint32_t sampling_rate) {
        sampling_rate_ = sampling_rate;
        update_phase_increment();
    }

    void set_frequency(const int32_t frequency) {
        frequency_ = frequency;
        update_phase_increment();
    }

    buffer_c16_t execute(const buffer_c16_t& src, const buffer_c16_t& dst) {
        auto phase = phase_;
        for (size_t i = 0; i < src.count; ++i) {
            const uint8_t index = phase >> 24;
            const uint8_t next = index + 1;
            const int32_t fraction = (phase >> 16) & 0xff;
            const uint32_t oscillator_first = oscillator_q15_[index];
            const uint32_t oscillator_next = oscillator_q15_[next];
            const int32_t sine_first =
                static_cast<int16_t>(oscillator_first >> 16);
            const int32_t cosine_first =
                static_cast<int16_t>(oscillator_first);
            const int32_t sine =
                sine_first +
                (((static_cast<int16_t>(oscillator_next >> 16) -
                   sine_first) *
                  fraction) >>
                 8);
            const int32_t cosine =
                cosine_first +
                (((static_cast<int16_t>(oscillator_next) -
                   cosine_first) *
                  fraction) >>
                 8);
            const uint32_t oscillator =
                static_cast<uint16_t>(cosine) |
                (static_cast<uint32_t>(
                     static_cast<uint16_t>(sine))
                 << 16);
            const uint32_t sample =
                *reinterpret_cast<const uint32_t*>(&src.p[i]);

            /* Two packed dual-16-bit multiplies implement
             * (I+jQ) * (cos-j sin). */
            const int32_t out_i =
                rounded_shift(__SMUAD(sample, oscillator), 15);
            const int32_t out_q =
                rounded_shift(__SMUSDX(oscillator, sample), 15);
            *reinterpret_cast<uint32_t*>(&dst.p[i]) =
                __PKHBT(__SSAT(out_i, 16), __SSAT(out_q, 16), 16);
            phase += phase_increment_;
        }
        phase_ = phase;
        return {dst.p, src.count, src.sampling_rate};
    }

   private:
    friend class FrequencyTranslatingDecimator32By8;

    static constexpr int32_t rounded_shift(
        const int32_t value,
        const uint32_t bits) {
        const int32_t rounding = int32_t{1} << (bits - 1);
        return value >= 0
                   ? (value + rounding) >> bits
                   : -((-value + rounding) >> bits);
    }

    void update_phase_increment() {
        if (sampling_rate_) {
            phase_increment_ = static_cast<uint32_t>(
                (static_cast<int64_t>(frequency_) * (int64_t{1} << 32)) /
                sampling_rate_);
        }
    }

    static constexpr std::array<int16_t, 256> sine_q15_{{
        0, 804, 1608, 2410, 3212, 4011, 4808, 5602, 6393, 7179, 7962, 8739, 9512, 10278, 11039, 11793,
        12539, 13279, 14010, 14732, 15446, 16151, 16846, 17530, 18204, 18868, 19519, 20159, 20787, 21403, 22005, 22594,
        23170, 23731, 24279, 24811, 25329, 25832, 26319, 26790, 27245, 27683, 28105, 28510, 28898, 29268, 29621, 29956,
        30273, 30571, 30852, 31113, 31356, 31580, 31785, 31971, 32137, 32285, 32412, 32521, 32609, 32678, 32728, 32757,
        32767, 32757, 32728, 32678, 32609, 32521, 32412, 32285, 32137, 31971, 31785, 31580, 31356, 31113, 30852, 30571,
        30273, 29956, 29621, 29268, 28898, 28510, 28105, 27683, 27245, 26790, 26319, 25832, 25329, 24811, 24279, 23731,
        23170, 22594, 22005, 21403, 20787, 20159, 19519, 18868, 18204, 17530, 16846, 16151, 15446, 14732, 14010, 13279,
        12539, 11793, 11039, 10278, 9512, 8739, 7962, 7179, 6393, 5602, 4808, 4011, 3212, 2410, 1608, 804,
        0, -804, -1608, -2410, -3212, -4011, -4808, -5602, -6393, -7179, -7962, -8739, -9512, -10278, -11039, -11793,
        -12539, -13279, -14010, -14732, -15446, -16151, -16846, -17530, -18204, -18868, -19519, -20159, -20787, -21403, -22005, -22594,
        -23170, -23731, -24279, -24811, -25329, -25832, -26319, -26790, -27245, -27683, -28105, -28510, -28898, -29268, -29621, -29956,
        -30273, -30571, -30852, -31113, -31356, -31580, -31785, -31971, -32137, -32285, -32412, -32521, -32609, -32678, -32728, -32757,
        -32767, -32757, -32728, -32678, -32609, -32521, -32412, -32285, -32137, -31971, -31785, -31580, -31356, -31113, -30852, -30571,
        -30273, -29956, -29621, -29268, -28898, -28510, -28105, -27683, -27245, -26790, -26319, -25832, -25329, -24811, -24279, -23731,
        -23170, -22594, -22005, -21403, -20787, -20159, -19519, -18868, -18204, -17530, -16846, -16151, -15446, -14732, -14010, -13279,
        -12539, -11793, -11039, -10278, -9512, -8739, -7962, -7179, -6393, -5602, -4808, -4011, -3212, -2410, -1608, -804,
    }};

    std::array<uint32_t, 256> oscillator_q15_{};
    uint32_t phase_{0};
    uint32_t phase_increment_{0};
    uint32_t sampling_rate_{192000};
    int32_t frequency_{0};
};

/*
 * Frequency-translating 32-tap FIR decimator.  Frequency translation is
 * split between coefficients modulated when tuning changes and a cheap
 * output-rate phase rotation.  This avoids running an NCO at the 384kHz
 * input rate.
 */
class FrequencyTranslatingDecimator32By8 {
   public:
    static constexpr size_t decimation_factor = 8;
    static constexpr size_t taps_count = 32;

    void configure(
        const std::array<int16_t, taps_count>& taps,
        const uint32_t input_sampling_rate) {
        taps_ = taps;
        input_sampling_rate_ = input_sampling_rate;
        decimator_.configure(complex_taps_, decimation_factor);
        output_xlator_.set_sample_rate(input_sampling_rate / decimation_factor);
        update_taps();
    }

    void set_frequency(const int32_t frequency) {
        frequency_ = frequency;
        output_xlator_.set_frequency(frequency);
        update_taps();
    }

    buffer_c16_t execute(
        const buffer_c16_t& src,
        const buffer_c16_t& dst) {
        const auto filtered = decimator_.execute(src, dst);
        return output_xlator_.execute(filtered, dst);
    }

   private:
    static void oscillator(
        const uint32_t phase,
        int32_t& sine,
        int32_t& cosine) {
        const uint8_t index = phase >> 24;
        const uint8_t next = index + 1;
        const uint8_t cosine_index = index + 64;
        const uint8_t cosine_next = cosine_index + 1;
        const int32_t fraction = (phase >> 16) & 0xff;
        const int32_t sine_first = FrequencyTranslator::sine_q15_[index];
        const int32_t cosine_first = FrequencyTranslator::sine_q15_[cosine_index];
        sine = sine_first +
               (((FrequencyTranslator::sine_q15_[next] - sine_first) * fraction) >> 8);
        cosine = cosine_first +
                 (((FrequencyTranslator::sine_q15_[cosine_next] - cosine_first) * fraction) >> 8);
    }

    void update_taps() {
        if (!input_sampling_rate_)
            return;

        const uint32_t tap_phase_increment = static_cast<uint32_t>(
            (static_cast<int64_t>(frequency_) * (int64_t{1} << 32)) /
            input_sampling_rate_);

        /* Centre the modulation on the FIR midpoint.  Besides changing
         * only a constant output phase, this makes the two coefficients
         * in each symmetric pair complex conjugates.  That property is
         * important after quantization: starting at tap zero accumulated
         * a one-sided phase and rounding error across the whole filter. */
        uint32_t phase = static_cast<uint32_t>(
            -((static_cast<int64_t>(
                    static_cast<int32_t>(tap_phase_increment)) *
                static_cast<int64_t>(taps_count - 1)) /
              2));
        for (size_t i = 0; i < taps_count; ++i) {
            int32_t sine;
            int32_t cosine;
            oscillator(phase, sine, cosine);
            /* FIRAndDecimateComplex uses Q16 coefficients; the source
             * real-tap filters use Q15 coefficients. */
            const int32_t tap = taps_[i];
            complex_taps_[i] = {
                static_cast<int16_t>(
                    FrequencyTranslator::rounded_shift(
                        tap * cosine, 14)),
                static_cast<int16_t>(
                    FrequencyTranslator::rounded_shift(
                        tap * sine, 14))};
            phase += tap_phase_increment;
        }
        decimator_.set_taps(complex_taps_);
    }

    std::array<int16_t, taps_count> taps_{};
    std::array<complex16_t, taps_count> complex_taps_{};
    decimate::FIRAndDecimateComplex decimator_{};
    FrequencyTranslator output_xlator_{};
    uint32_t input_sampling_rate_{0};
    int32_t frequency_{0};
};

} /* namespace dsp */

#endif /*__DSP_FREQUENCY_XLATOR_H__*/
