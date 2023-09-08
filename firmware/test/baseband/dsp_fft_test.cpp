/*
 * Copyright (C) 2023 Bernd Herzog
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

#include "dsp_fft.hpp"
#include "doctest.h"

TEST_CASE("ifft successfully calculates dc on zero frequency") {
    uint32_t fft_width = 8;
    complex16_t* v = new complex16_t[fft_width];
    complex16_t* tmp = new complex16_t[fft_width];

    v[0] = {1024, 0};  // DC bin
    v[1] = {0, 0};
    v[2] = {0, 0};
    v[3] = {0, 0};
    v[4] = {0, 0};
    v[5] = {0, 0};
    v[6] = {0, 0};
    v[7] = {0, 0};

    ifft<complex16_t>(v, fft_width, tmp);

    CHECK(v[0].real() == 1024);
    CHECK(v[1].real() == 1024);
    CHECK(v[2].real() == 1024);
    CHECK(v[3].real() == 1024);
    CHECK(v[4].real() == 1024);
    CHECK(v[5].real() == 1024);
    CHECK(v[6].real() == 1024);
    CHECK(v[7].real() == 1024);

    for (uint32_t i = 0; i < fft_width; i++)
        CHECK(v[i].imag() == 0);

    delete[] v;
    delete[] tmp;
}

TEST_CASE("ifft successfully calculates sine of quarter the sample rate") {
    uint32_t fft_width = 8;
    complex16_t* v = new complex16_t[fft_width];
    complex16_t* tmp = new complex16_t[fft_width];

    v[0] = {0, 0};
    v[1] = {0, 0};
    v[2] = {1024, 0};  // sample rate /4 bin
    v[3] = {0, 0};
    v[4] = {0, 0};
    v[5] = {0, 0};
    v[6] = {0, 0};
    v[7] = {0, 0};

    ifft<complex16_t>(v, fft_width, tmp);

    CHECK(v[0].real() == 1024);
    CHECK(v[1].real() == 0);
    CHECK(v[2].real() == -1024);
    CHECK(v[3].real() == 0);
    CHECK(v[4].real() == 1024);
    CHECK(v[5].real() == 0);
    CHECK(v[6].real() == -1024);
    CHECK(v[7].real() == 0);

    CHECK(v[0].imag() == 0);
    CHECK(v[1].imag() == 1024);
    CHECK(v[2].imag() == 0);
    CHECK(v[3].imag() == -1024);
    CHECK(v[4].imag() == 0);
    CHECK(v[5].imag() == 1024);
    CHECK(v[6].imag() == 0);
    CHECK(v[7].imag() == -1024);

    delete[] v;
    delete[] tmp;
}

TEST_CASE("ifft successfully calculates pure sine of half the sample rate") {
    uint32_t fft_width = 8;
    complex16_t* v = new complex16_t[fft_width];
    complex16_t* tmp = new complex16_t[fft_width];

    v[0] = {0, 0};
    v[1] = {0, 0};
    v[2] = {0, 0};
    v[3] = {0, 0};
    v[4] = {1024, 0};  // sample rate /2 bin
    v[5] = {0, 0};
    v[6] = {0, 0};
    v[7] = {0, 0};

    ifft<complex16_t>(v, fft_width, tmp);

    CHECK(v[0].real() == 1024);
    CHECK(v[1].real() == -1024);
    CHECK(v[2].real() == 1024);
    CHECK(v[3].real() == -1024);
    CHECK(v[4].real() == 1024);
    CHECK(v[5].real() == -1024);
    CHECK(v[6].real() == 1024);
    CHECK(v[7].real() == -1024);

    for (uint32_t i = 0; i < fft_width; i++)
        CHECK(v[i].imag() == 0);

    delete[] v;
    delete[] tmp;
}
