/*
 * Copyright (C) 2026 zxkmm
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

#include "spec_an_dsp.hpp"
#include "doctest.h"

#include <complex>
#include <vector>

using namespace spec_an;
using namespace spec_an::dsp;

namespace {

constexpr double two_pi = 6.283185307179586476925;

std::vector<float> sin_table() {
    std::vector<float> t(kMaxFft / 4 + 1);
    init_sin_table(t.data());
    return t;
}

/* Complex tone at `cycles` per block (may be fractional), amplitude `a`. */
std::vector<int8_t> tone(size_t n, double cycles, double a, double phase = 0.3) {
    std::vector<int8_t> iq(2 * n);
    for (size_t i = 0; i < n; i++) {
        const double ph = two_pi * cycles * i / n + phase;
        iq[2 * i] = static_cast<int8_t>(std::lround(a * std::cos(ph)));
        iq[2 * i + 1] = static_cast<int8_t>(std::lround(a * std::sin(ph)));
    }
    return iq;
}

/* Power in one bin, in dB relative to a full scale tone, as the M4 does it. */
float bin_db(const std::vector<int8_t>& iq, size_t log2n, Window w, int32_t bin) {
    static const auto sin_q = sin_table();
    std::vector<CFloat> x(size_t{1} << log2n);
    load_block(x.data(), iq.data(), log2n);
    fft(x.data(), log2n, sin_q.data());
    float acc = 0.0f;
    window_power(x.data(), x.size(), bin, 1, w, &acc);
    return 4.34294482f * ln_approx(acc) + norm_db(x.size(), w, 1);
}

}  // namespace

TEST_CASE("spec_an ln_approx is within 0.001 dB over the useful range") {
    for (float x = 1e-12f; x < 1e12f; x *= 1.37f) {
        const float err_db = 4.34294482f * (ln_approx(x) - std::log(x));
        CHECK(std::fabs(err_db) < 0.001f);
    }
}

TEST_CASE("spec_an fft matches a direct DFT") {
    const auto sin_q = sin_table();
    for (size_t log2n = kMinFftLog2; log2n <= kMaxFftLog2; log2n++) {
        const size_t n = size_t{1} << log2n;
        std::vector<int8_t> iq(2 * n);
        uint32_t lcg = 12345;
        for (auto& v : iq) {
            lcg = lcg * 1103515245u + 12345u;
            v = static_cast<int8_t>((lcg >> 16) & 0xff);
        }
        std::vector<CFloat> x(n);
        load_block(x.data(), iq.data(), log2n);
        fft(x.data(), log2n, sin_q.data());

        double worst = 0.0;
        for (size_t k = 0; k < n; k += 37) {
            std::complex<double> sum{0.0, 0.0};
            for (size_t i = 0; i < n; i++) {
                sum += std::complex<double>(iq[2 * i], iq[2 * i + 1]) * std::polar(1.0, -two_pi * double(k) * double(i) / double(n));
            }
            const double err = std::abs(std::complex<double>(x[k].re, x[k].im) - sum);
            worst = std::max(worst, err / (127.0 * n));
        }
        CHECK(worst < 1e-5);
    }
}

TEST_CASE("spec_an full scale tone centred in a bin reads 0 dB in every window") {
    for (int w = 0; w < static_cast<int>(Window::Count); w++) {
        for (size_t log2n = kMinFftLog2; log2n <= kMaxFftLog2; log2n++) {
            const size_t n = size_t{1} << log2n;
            const int32_t bin = static_cast<int32_t>(n / 5);
            const float db = bin_db(tone(n, bin, 127.0), log2n, static_cast<Window>(w), bin);
            CHECK(std::fabs(db) < 0.02f);
        }
    }
}

TEST_CASE("spec_an negative frequencies land in the wrapped bins") {
    const size_t log2n = 9;
    const size_t n = size_t{1} << log2n;
    const float db = bin_db(tone(n, -40.0, 127.0), log2n, Window::BlackmanHarris, -40);
    CHECK(std::fabs(db) < 0.02f);
    const float img = bin_db(tone(n, -40.0, 127.0), log2n, Window::BlackmanHarris, 40);
    CHECK(img < -60.0f);
}

TEST_CASE("spec_an scalloping loss matches the window") {
    /* Tone half way between two bins: worst case. */
    const size_t log2n = 10;
    const size_t n = size_t{1} << log2n;
    const auto iq = tone(n, 200.5, 127.0);
    CHECK(bin_db(iq, log2n, Window::Rect, 200) == doctest::Approx(-3.92f).epsilon(0.03));
    CHECK(bin_db(iq, log2n, Window::Hann, 200) == doctest::Approx(-1.42f).epsilon(0.03));
    CHECK(bin_db(iq, log2n, Window::BlackmanHarris, 200) == doctest::Approx(-0.83f).epsilon(0.03));
    CHECK(std::fabs(bin_db(iq, log2n, Window::FlatTop, 200)) < 0.05f);
}

/* Whole sweep through the same steps as the M0 plan and the M4 worker: a
 * tone must come out at the display point for its frequency, and every
 * point must be written exactly once per sweep. */
struct SweepCase {
    uint64_t start;
    uint64_t span;
    uint32_t fs;
    size_t log2n;
    BinLayout layout;
    uint64_t tone;
};

void run_sweep(const SweepCase& c) {
    static const auto sin_q = sin_table();
    const size_t n = size_t{1} << c.log2n;
    const uint64_t coverage = c.span + c.span / (kPoints - 1);
    const uint32_t k_max = layout_count(c.layout, n);
    const uint64_t bins_needed = (coverage * n + c.fs - 1) / c.fs + 3;
    const uint32_t slices = (bins_needed + k_max - 1) / k_max;
    const uint32_t k = (bins_needed + slices - 1) / slices;
    const int32_t first = layout_first(c.layout, n);
    const int64_t f_bin0 = static_cast<int64_t>(c.start) - static_cast<int64_t>(c.span / (2 * (kPoints - 1)));
    const float px_per_bin = float(uint64_t(c.fs) * (kPoints - 1)) / float(c.span << c.log2n);

    PointDetector det;
    det.configure(px_per_bin, Detector::Peak, norm_db(n, Window::BlackmanHarris, 1), slices);

    std::vector<int16_t> out(kPoints, kNoData);
    std::vector<int> written(kPoints, 0);
    std::vector<CFloat> x(n);
    std::vector<float> acc(k);
    uint32_t noise = 1;

    for (uint32_t s = 0; s < slices; s++) {
        const int64_t bins = int64_t(s) * k - first;
        const int64_t lo = f_bin0 + ((bins * c.fs) >> c.log2n);
        /* Front end: the anti-alias filter only lets |offset| < 0.45 fs in. */
        const double offset = double(int64_t(c.tone) - lo);
        std::vector<int8_t> iq(2 * n);
        for (size_t i = 0; i < n; i++) {
            noise = noise * 1103515245u + 12345u;
            double re = ((noise >> 16) & 3) - 1.5;
            double im = ((noise >> 20) & 3) - 1.5;
            if (std::fabs(offset) < 0.45 * c.fs) {
                const double ph = two_pi * offset * i / c.fs;
                re += 100.0 * std::cos(ph);
                im += 100.0 * std::sin(ph);
            }
            iq[2 * i] = static_cast<int8_t>(std::lround(re));
            iq[2 * i + 1] = static_cast<int8_t>(std::lround(im));
        }
        load_block(x.data(), iq.data(), c.log2n);
        fft(x.data(), c.log2n, sin_q.data());
        std::fill(acc.begin(), acc.end(), 0.0f);
        window_power(x.data(), n, first, k, Window::BlackmanHarris, acc.data());
        det.run(acc.data(), k, s, 1, out.data());
        for (int px = det.px_begin(); px < det.px_end(); px++) written[px]++;
    }

    for (size_t px = 0; px < kPoints; px++) {
        CHECK(written[px] == 1);
        CHECK(out[px] != kNoData);
    }

    const size_t peak = std::max_element(out.begin(), out.end()) - out.begin();
    const double expected = double(c.tone - c.start) * (kPoints - 1) / double(c.span);
    CHECK(std::fabs(double(peak) - expected) <= 1.0);
    /* 100/127 of full scale = -2.1 dB, less up to 0.83 dB of scalloping. */
    CHECK(out[peak] / 100.0f > -3.2f);
    CHECK(out[peak] / 100.0f < -1.9f);
}

TEST_CASE("spec_an wide sweep, upper sideband, places the tone") {
    run_sweep({400'000'000, 100'000'000, 20'000'000, 8, BinLayout::Upper, 433'920'000});
}

TEST_CASE("spec_an wide sweep, both sidebands, places the tone") {
    run_sweep({400'000'000, 100'000'000, 20'000'000, 8, BinLayout::Both, 433'920'000});
}

TEST_CASE("spec_an single tune, dense bins, places the tone") {
    run_sweep({430'000'000, 5'000'000, 20'000'000, 11, BinLayout::Upper, 431'234'567});
}

TEST_CASE("spec_an narrow span, sparse bins, places the tone") {
    run_sweep({433'800'000, 200'000, 2'500'000, 11, BinLayout::Upper, 433'921'000});
}

TEST_CASE("spec_an narrow span, both sidebands, places the tone") {
    run_sweep({433'800'000, 200'000, 2'500'000, 11, BinLayout::Both, 433'871'000});
}

TEST_CASE("spec_an re-running a slice rewinds the detector") {
    PointDetector det;
    det.configure(0.25f, Detector::Average, 0.0f, 4);
    std::vector<float> p(100, 1.0f);
    std::vector<int16_t> a(kPoints, kNoData), b(kPoints, kNoData);

    det.run(p.data(), p.size(), 0, 7, a.data());
    det.run(p.data(), p.size(), 1, 7, a.data());
    const uint16_t begin = det.px_begin(), end = det.px_end();
    p.assign(100, 4.0f);
    det.run(p.data(), p.size(), 1, 7, b.data()); /* same slice, same sweep */
    CHECK(det.px_begin() == begin);
    CHECK(det.px_end() == end);
    /* The rewound run must not have mixed in the first attempt's power. */
    CHECK(b[begin + 1] == doctest::Approx(4.34294482f * std::log(4.0f) * 100.0f).epsilon(0.01));
}
