/*
 * Copyright (C) 2026
 *
 * Host golden: TJpgDec (Mayhem vendored tjpgd.c + tjpgdcnf.h) decodes
 * `tools/meteor_lrpt/data/g4_tiny_baseline.jpg` (8x8 RGB baseline from PIL).
 * Regenerate constants: `tools/meteor_lrpt/docker_gen_tjpgd_golden.sh` in Docker.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>

extern "C" {
#include "tjpgd.h"
}

namespace {

constexpr unsigned kExpW = 8;
constexpr unsigned kExpH = 8;
constexpr uint32_t kExpAcc = 3380314112u;

struct Stream {
    const uint8_t* p{};
    size_t n{}, pos{};
};

struct Session {
    Stream stream{};
    uint32_t acc{0};
};

size_t infunc(JDEC* jd, uint8_t* buf, size_t sz) {
    auto* ses = static_cast<Session*>(jd->device);
    Stream& s = ses->stream;
    size_t m = sz;
    if (m > s.n - s.pos)
        m = s.n - s.pos;
    if (buf == nullptr) {
        s.pos += m;
        return m;
    }
    for (size_t i = 0; i < m; i++)
        buf[i] = s.p[s.pos++];
    return m;
}

int out_accum(JDEC* jd, void* bitmap, JRECT* rect) {
    auto* ses = static_cast<Session*>(jd->device);
    auto* rgb = static_cast<const uint8_t*>(bitmap);
    uint32_t& acc = ses->acc;
    const unsigned w = rect->right - rect->left + 1;
    const unsigned h = rect->bottom - rect->top + 1;
    for (unsigned r = 0; r < h; r++) {
        for (unsigned x = 0; x < w; x++) {
            for (int c = 0; c < 3; c++)
                acc = acc * 1315423911u + rgb[(r * w + x) * 3u + (unsigned)c];
        }
    }
    return 1;
}

bool read_all(const char* path, std::vector<uint8_t>& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f)
        return false;
    f.seekg(0, std::ios::end);
    const auto n = f.tellg();
    if (n <= 0 || n > 4 * 1024 * 1024)
        return false;
    f.seekg(0);
    out.resize((size_t)n);
    f.read(reinterpret_cast<char*>(out.data()), n);
    return (bool)f;
}

}  // namespace

int main(int argc, char** argv) {
    const char* jpath = (argc >= 2) ? argv[1] : "tools/meteor_lrpt/data/g4_tiny_baseline.jpg";
    std::vector<uint8_t> jpg;
    if (!read_all(jpath, jpg)) {
        std::cerr << "read fail: " << jpath << "\n";
        return 2;
    }

    Session ses{};
    ses.stream = {jpg.data(), jpg.size(), 0};
    JDEC jd{};
    jd.device = &ses;
    alignas(8) static uint8_t pool[4096];
    if (jd_prepare(&jd, infunc, pool, sizeof pool, &ses) != JDR_OK) {
        std::cerr << "jd_prepare fail\n";
        return 1;
    }
    if (jd.width != kExpW || jd.height != kExpH) {
        std::cerr << "dims " << jd.width << "x" << jd.height << " expected " << kExpW << "x" << kExpH << "\n";
        return 1;
    }

    ses.stream.pos = 0;
    ses.acc = 0;
    if (jd_prepare(&jd, infunc, pool, sizeof pool, &ses) != JDR_OK) {
        std::cerr << "jd_prepare2 fail\n";
        return 1;
    }
    if (jd_decomp(&jd, out_accum, 0) != JDR_OK) {
        std::cerr << "jd_decomp fail\n";
        return 1;
    }
    if (ses.acc != kExpAcc) {
        std::cerr << "acc " << ses.acc << " expected " << kExpAcc << " (regen with docker_gen_tjpgd_golden.sh)\n";
        return 1;
    }

    std::cout << "tjpgd_g4_host_ok\n";
    return 0;
}
