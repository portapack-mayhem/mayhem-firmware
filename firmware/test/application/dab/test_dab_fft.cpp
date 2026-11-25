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

#include "doctest.h"

#include "dab/dab_fft.hpp"
#include "dab/dab_constants.hpp"

#include <cmath>
#include <complex>
#include <cstdint>
#include <vector>

namespace {

constexpr double kTwoPi = 6.283185307179586;

std::vector<dab::FftComplex> tone(size_t bin, double amplitude) {
    std::vector<dab::FftComplex> v(dab::kFftSize);
    for (size_t n = 0; n < dab::kFftSize; n++) {
        const double p = kTwoPi * bin * n / dab::kFftSize;
        v[n].re = static_cast<int16_t>(std::lround(amplitude * std::cos(p)));
        v[n].im = static_cast<int16_t>(std::lround(amplitude * std::sin(p)));
    }
    return v;
}

double magnitude(const dab::FftComplex& c) {
    return std::hypot(static_cast<double>(c.re), static_cast<double>(c.im));
}

/* Straight DFT, for comparison. Only used on small subsets of bins. */
std::complex<double> reference_bin(const std::vector<dab::FftComplex>& in, size_t k) {
    std::complex<double> acc{0.0, 0.0};
    for (size_t n = 0; n < in.size(); n++) {
        const double p = -kTwoPi * k * n / in.size();
        const std::complex<double> s{static_cast<double>(in[n].re), static_cast<double>(in[n].im)};
        acc += s * std::complex<double>{std::cos(p), std::sin(p)};
    }
    return acc;
}

}  // namespace

TEST_CASE("bit reversal is an involution over the transform size") {
    for (size_t i = 0; i < dab::kFftSize; i++) {
        const size_t r = dab::fft_bit_reverse_index(i);
        CHECK(r < dab::kFftSize);
        CHECK(dab::fft_bit_reverse_index(r) == i);
    }
}

TEST_CASE("bit reversal known values") {
    CHECK(dab::fft_bit_reverse_index(0) == 0);
    CHECK(dab::fft_bit_reverse_index(1) == 1024);
    CHECK(dab::fft_bit_reverse_index(2) == 512);
    CHECK(dab::fft_bit_reverse_index(2047) == 2047);
}

TEST_CASE("null input is refused") {
    CHECK_FALSE(dab::fft2048(nullptr));
}

TEST_CASE("a single tone lands in exactly one bin") {
    for (size_t bin : {1u, 137u, 512u, 1500u}) {
        auto data = tone(bin, 8000.0);
        REQUIRE(dab::fft2048(data.data()));

        size_t peak_bin = 0;
        double peak = 0.0;
        for (size_t i = 0; i < dab::kFftSize; i++) {
            const double m = magnitude(data[i]);
            if (m > peak) {
                peak = m;
                peak_bin = i;
            }
        }

        REQUIRE(peak_bin == bin);

        /* Every other bin must be far below the peak. */
        double next = 0.0;
        for (size_t i = 0; i < dab::kFftSize; i++) {
            if (i == bin)
                continue;
            next = std::max(next, magnitude(data[i]));
        }
        CHECK(next < peak * 0.05);
    }
}

TEST_CASE("block floating point reports a usable scale factor") {
    // The unscaled peak of a tone of amplitude A is N*A; the reported shift
    // must reconstruct that.
    auto data = tone(300, 8000.0);
    uint8_t shift = 0;
    REQUIRE(dab::fft2048(data.data(), &shift));

    const double peak = magnitude(data[300]) * std::pow(2.0, shift);
    const double expected = 8000.0 * dab::kFftSize;
    CHECK(peak > expected * 0.8);
    CHECK(peak < expected * 1.2);
}

TEST_CASE("no scaling is applied when the input cannot overflow") {
    // A tiny input never reaches half scale, so no bits should be discarded.
    auto data = tone(300, 4.0);
    uint8_t shift = 0;
    REQUIRE(dab::fft2048(data.data(), &shift));
    CHECK(shift < dab::kFftSize);
}

TEST_CASE("two tones are resolved with the right amplitude ratio") {
    std::vector<dab::FftComplex> data(dab::kFftSize);
    for (size_t n = 0; n < dab::kFftSize; n++) {
        const double pa = kTwoPi * 200 * n / dab::kFftSize;
        const double pb = kTwoPi * 900 * n / dab::kFftSize;
        data[n].re = static_cast<int16_t>(std::lround(6000.0 * std::cos(pa) + 3000.0 * std::cos(pb)));
        data[n].im = static_cast<int16_t>(std::lround(6000.0 * std::sin(pa) + 3000.0 * std::sin(pb)));
    }

    REQUIRE(dab::fft2048(data.data()));

    const double a = magnitude(data[200]);
    const double b = magnitude(data[900]);
    CHECK(a > b);
    CHECK(b / a > 0.4);
    CHECK(b / a < 0.6);
}

TEST_CASE("agrees with a direct DFT to better than 40 dB") {
    // A broadband input exercises every stage rather than a single twiddle.
    std::vector<dab::FftComplex> input(dab::kFftSize);
    uint32_t x = 22222;
    for (size_t n = 0; n < dab::kFftSize; n++) {
        x = x * 1103515245u + 12345u;
        input[n].re = static_cast<int16_t>((static_cast<int32_t>((x >> 16) & 0x7FF)) - 1024);
        x = x * 1103515245u + 12345u;
        input[n].im = static_cast<int16_t>((static_cast<int32_t>((x >> 16) & 0x7FF)) - 1024);
    }

    auto data = input;
    uint8_t shift = 0;
    REQUIRE(dab::fft2048(data.data(), &shift));
    const double scale = std::pow(2.0, shift);

    double signal = 0.0;
    double error = 0.0;

    for (size_t k = 0; k < dab::kFftSize; k += 61) {
        const auto ref = reference_bin(input, k);
        const std::complex<double> got{static_cast<double>(data[k].re) * scale,
                                       static_cast<double>(data[k].im) * scale};

        signal += std::norm(ref);
        error += std::norm(got - ref);
    }

    REQUIRE(signal > 0.0);
    const double snr_db = 10.0 * std::log10(signal / error);
    CHECK(snr_db > 40.0);
}

TEST_CASE("a zero input stays zero") {
    std::vector<dab::FftComplex> data(dab::kFftSize);
    REQUIRE(dab::fft2048(data.data()));
    for (const auto& c : data) {
        CHECK(c.re == 0);
        CHECK(c.im == 0);
    }
}

TEST_CASE("DC input concentrates in bin zero") {
    std::vector<dab::FftComplex> data(dab::kFftSize);
    for (auto& c : data)
        c = dab::FftComplex{4000, 0};

    REQUIRE(dab::fft2048(data.data()));

    CHECK(magnitude(data[0]) > 1000.0);
    for (size_t k = 1; k < dab::kFftSize; k++)
        REQUIRE(magnitude(data[k]) < 50.0);
}
