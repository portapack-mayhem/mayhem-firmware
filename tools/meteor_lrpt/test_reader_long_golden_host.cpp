/*
 * Host-only: one long-path MeteorDeinterleaverReader::read_samples(8192) + deterministic soft input,
 * then FNV-1a 64-bit digest over the full deinterleaver ring (SatDump-sized).
 *
 * Golden refresh (Linux / WSL / Git Bash, from repo root):
 *   g++ -std=c++17 -O2 -I firmware/baseband -I firmware/common \
 *       tools/meteor_lrpt/test_reader_long_golden_host.cpp \
 *       firmware/baseband/meteor_lrpt/external_ring_ram.cpp \
 *       firmware/baseband/meteor_lrpt/meteor_deinterleaver.cpp \
 *       firmware/baseband/meteor_lrpt/meteor_lrpt_deinterleave_dispatch.cpp \
 *       firmware/baseband/meteor_lrpt/meteor_deinterleaver_reader.cpp \
 *       firmware/baseband/meteor_lrpt/meteor_soft_correlate.cpp \
 *       -o /tmp/reader_long_golden && /tmp/reader_long_golden --dump-golden
 * Then paste the printed u64 hex into kExpectedRingFnv1a64 below.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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

constexpr uint64_t kFnvOffset = 14695981039346656037ULL;
constexpr uint64_t kFnvPrime = 1099511628211ULL;

uint64_t fnv1a64_ring(const meteor_lrpt::IMeteorDeintRing& ring, size_t nbytes) {
    uint64_t h = kFnvOffset;
    for (size_t i = 0; i < nbytes; i++) {
        const int8_t b = ring.read_byte((uint32_t)i);
        h ^= (uint8_t)b;
        h *= kFnvPrime;
    }
    return h;
}

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

/* From: g++ ... && ./a.out --dump-golden (Ubuntu noble host); do not change without re-dump. */
constexpr uint64_t kExpectedRingFnv1a64 = 3908576911922097922ULL;

}  // namespace

int main(int argc, char** argv) {
    const bool dump = (argc >= 2) && (std::strcmp(argv[1], "--dump-golden") == 0);

    /* Deterministic soft stream (extend with same PRNG-like rule if reads exceed len). */
    constexpr size_t kOutLen = 8192;
    const int br0 = 0;
    const size_t ns = meteor_lrpt::meteor_deinterleave_num_samples(kOutLen, br0);
    /* Extra tail for sync_offs > 0 follow-up read inside read_samples. */
    std::vector<int8_t> soft_in(ns + meteor_lrpt::kInterMarkerStride + 2048);
    for (size_t i = 0; i < soft_in.size(); i++)
        soft_in[i] = (int8_t)((int)((i * 1103515245u + 12345u) % 251u) - 125);

    meteor_lrpt::MeteorDeintRamRing ring;
    StreamCtx sc{&soft_in, 0};
    /* dst must cover rotate + deinterleave scratch per reader implementation */
    const size_t dst_cap = std::max(ns, kOutLen) + (size_t)meteor_lrpt::kInterMarkerStride + 4096;
    std::vector<int8_t> dst(dst_cap, 0);

    std::array<uint8_t, meteor_lrpt::kDeinterleaverReaderHardScratchBytes> reader_hard{};
    meteor_lrpt::MeteorDeinterleaverReader reader(ring, reader_hard.data());
    const int rc = reader.read_samples(stream_read, &sc, dst.data(), kOutLen);
    if (rc != 0) {
        std::fprintf(stderr, "read_samples failed rc=%d consumed=%zu / %zu\n", rc, sc.pos, soft_in.size());
        return 2;
    }

    const uint64_t h = fnv1a64_ring(ring, meteor_lrpt::kDeinterleaverRingBytes);
    if (dump) {
        std::printf("ring_fnv1a64=%lluULL\n", (unsigned long long)h);
        std::printf("// rotation=%u ring_off=%u branch=%d consumed=%zu\n", reader.rotation(), reader.ring_offset(),
                     reader.ring_branch(), sc.pos);
        return 0;
    }

    if (h != kExpectedRingFnv1a64) {
        std::fprintf(stderr, "ring FNV mismatch: got %llu expected %llu\n", (unsigned long long)h,
                     (unsigned long long)kExpectedRingFnv1a64);
        return 4;
    }

    std::printf("reader_long_golden ok fnv1a=%llu rotation=%u\n", (unsigned long long)h, reader.rotation());
    return 0;
}
