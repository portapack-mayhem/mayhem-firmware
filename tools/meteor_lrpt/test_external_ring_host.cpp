/*
 * Host-only regression: RAM ring + meteor_deinterleave + MeteorDeinterleaverReader::read_samples (short path).
 * Build (from repo root, example):
 *   g++ -std=c++17 -O2 -I firmware/baseband -I firmware/common tools/meteor_lrpt/test_external_ring_host.cpp \
 *       firmware/baseband/meteor_lrpt/external_ring_ram.cpp \
 *       firmware/baseband/meteor_lrpt/meteor_deinterleaver.cpp \
 *       firmware/baseband/meteor_lrpt/meteor_lrpt_deinterleave_dispatch.cpp \
 *       firmware/baseband/meteor_lrpt/meteor_deinterleaver_reader.cpp \
 *       firmware/baseband/meteor_lrpt/meteor_soft_correlate.cpp \
 *       -o /tmp/meteor_ring_test
 *   /tmp/meteor_ring_test
 */
#include "meteor_lrpt/external_ring.hpp"
#include "meteor_lrpt/meteor_deinterleaver.hpp"
#include "meteor_lrpt/meteor_deinterleaver_reader.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

struct StreamCtx {
    const std::vector<int8_t>* data{};
    size_t pos{0};
};

bool stream_read(void* ctx, int8_t* buf, size_t len) {
    auto* s = static_cast<StreamCtx*>(ctx);
    if (s->pos + len > s->data->size())
        return false;
    std::memcpy(buf, s->data->data() + s->pos, len);
    s->pos += len;
    return true;
}

}  // namespace

int main() {
    meteor_lrpt::MeteorDeintRamRing ring;
    std::vector<int8_t> src(9000, 3);
    std::vector<int8_t> dst(8192, 0);
    uint32_t off = 0;
    int br = 0;
    meteor_lrpt::meteor_deinterleave(ring, dst.data(), src.data(), 8192, off, br);
    std::printf("deinterleave ok off=%u br=%d dst0=%d\n", off, br, (int)dst[0]);

    /* Short-path read_samples: len=64 => num_samples=72 (< kInterMarkerStride*8). */
    const size_t out_len = 64;
    const size_t ns = meteor_lrpt::meteor_deinterleave_num_samples(out_len, 0);
    std::vector<int8_t> soft_in(ns, 7);
    StreamCtx sc{&soft_in, 0};
    std::vector<int8_t> rsbuf(std::max(out_len, ns) + 256, 0);
    std::array<uint8_t, meteor_lrpt::kDeinterleaverReaderHardScratchBytes> reader_hard{};
    meteor_lrpt::MeteorDeinterleaverReader reader(ring, reader_hard.data());
    const int rc = reader.read_samples(stream_read, &sc, rsbuf.data(), out_len);
    std::printf("read_samples short-path rc=%d consumed=%zu\n", rc, sc.pos);
    return rc != 0 ? 2 : 0;
}
