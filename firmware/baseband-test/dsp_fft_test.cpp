#include "dsp_fft.hpp"
#include "doctest.h"

TEST_CASE("ifft") {
    uint32_t fft_width = 8;
    complex16_t* v = new complex16_t[fft_width];
    complex16_t* tmp = new complex16_t[fft_width];

    v[0] = {0, 0};
    v[1] = {0, 0};
    v[2] = {0, 0};
    v[3] = {0, 0};
    v[4] = {1024, 0};
    v[5] = {0, 0};
    v[6] = {0, 0};
    v[7] = {0, 0};

    tmp[0] = {0, 0};
    tmp[1] = {0, 0};
    tmp[2] = {0, 0};
    tmp[3] = {0, 0};
    tmp[4] = {0, 0};
    tmp[5] = {0, 0};
    tmp[6] = {0, 0};
    tmp[7] = {0, 0};

    ifft<complex16_t>(v, fft_width, tmp);

    CHECK(v[0].real() == 1024);
    CHECK(v[1].real() == -1024);
    CHECK(v[2].real() == 1024);
    CHECK(v[3].real() == -1024);
    CHECK(v[4].real() == 1024);
    CHECK(v[5].real() == -1024);
    CHECK(v[6].real() == 1024);
    CHECK(v[7].real() == -1024);

    free(v);
    free(tmp);
}
