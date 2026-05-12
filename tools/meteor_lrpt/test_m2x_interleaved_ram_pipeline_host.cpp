/*
 * Host: dual MeteorDeintRamRing + two readers (SatDump-style A / PHASE_90 B) + M2xInterleavedPostDeintPipeline
 * + derandomize + ASM + RS, streaming 8192-byte soft blocks (same layout as on-device M0+M4 interleaved path).
 *
 * Build (see run_reader_long_golden_docker.sh for full g++ line).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "meteor_lrpt/ccsds_derandomize.hpp"
#include "meteor_lrpt/external_ring.hpp"
#include "meteor_lrpt/meteor_deinterleaver.hpp"
#include "meteor_lrpt/meteor_deinterleaver_reader.hpp"
#include "meteor_lrpt/meteor_m2x_interleaved.hpp"
#include "meteor_lrpt/meteor_m2x_interleaved_pipeline.hpp"
#include "meteor_lrpt/rs223_decode.hpp"

#include <array>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

static constexpr uint8_t kCaduAsm[] = {0x1A, 0xCF, 0xFC, 0x1D};

struct StreamCtx {
    const int8_t* p{nullptr};
    size_t rem{0};
};

bool stream_pull(void* ctx, int8_t* buf, size_t len) {
    auto* c = static_cast<StreamCtx*>(ctx);
    if (!c || c->rem < len)
        return false;
    std::memcpy(buf, c->p, len);
    c->p += len;
    c->rem -= len;
    return true;
}

bool post_fec_frame_checks(std::array<uint8_t, 1024>& frame) {
    meteor_derand_ccsds(&frame[4], 1020);
    if (frame[9] == 0xff) {
        for (size_t i = 0; i < frame.size(); i++)
            frame[i] ^= 0xff;
    }
    return std::memcmp(&frame[4], kCaduAsm, sizeof(kCaduAsm)) == 0;
}

void fill_deterministic_soft(std::vector<int8_t>& out, size_t nbytes) {
    out.resize(nbytes);
    for (size_t i = 0; i < nbytes; i++)
        out[i] = (int8_t)((int)((i * 1103515245u + 12345u) % 251u) - 125);
}

constexpr size_t kSoftSlackTail = 16384u;

}  // namespace

int main(int argc, char** argv) {
    std::vector<int8_t> soft_all;
    if (argc >= 2 && std::strcmp(argv[1], "-") != 0) {
        FILE* f = std::fopen(argv[1], "rb");
        if (!f) {
            std::fprintf(stderr, "open %s failed\n", argv[1]);
            return 2;
        }
        int ch;
        while ((ch = std::fgetc(f)) != EOF)
            soft_all.push_back((int8_t)(unsigned char)ch);
        std::fclose(f);
        if (soft_all.size() % 8192u != 0) {
            std::fprintf(stderr, "file soft length %zu not multiple of 8192\n", soft_all.size());
            return 3;
        }
        soft_all.resize(soft_all.size() + kSoftSlackTail, 0);
    } else {
        fill_deterministic_soft(soft_all, 8192u * 64u + kSoftSlackTail);
    }

    if (soft_all.size() < kSoftSlackTail + 8192u) {
        std::fprintf(stderr, "soft buffer too small\n");
        return 3;
    }
    const size_t soft_usable = soft_all.size() - kSoftSlackTail;

    if (soft_usable % 8192u != 0) {
        std::fprintf(stderr, "usable soft length %zu not multiple of 8192\n", soft_usable);
        return 3;
    }

    meteor_lrpt::MeteorDeintRamRing ring_a;
    meteor_lrpt::MeteorDeintRamRing ring_b;
    std::array<uint8_t, meteor_lrpt::kDeinterleaverReaderHardScratchBytes> reader_hard{};
    meteor_lrpt::MeteorDeinterleaverReader reader_a(ring_a, reader_hard.data(), nullptr);
    meteor_lrpt::MeteorDeinterleaverReader reader_b(ring_b, reader_hard.data(), nullptr);

    constexpr size_t kBWork = meteor_lrpt::meteor_deinterleave_num_samples(8192, 0) + (size_t)(meteor_lrpt::kInterMarkerStride / 2) + 32u;
    std::array<int8_t, 8192> deint_a{};
    std::vector<int8_t> deint_b_work(kBWork);

    meteor_lrpt::M2xInterleavedPostDeintPipeline pipe{};
    std::vector<uint8_t> cadu_stream;
    size_t blocks = 0;

    for (size_t off = 0; off + 8192u <= soft_usable; off += 8192u) {
        const int8_t* chunk = soft_all.data() + off;
        StreamCtx ca{chunk, soft_all.size() - off};
        const int ra = reader_a.read_samples(stream_pull, &ca, deint_b_work.data(), 8192);
        if (ra != 0) {
            std::fprintf(stderr, "reader_a rc=%d off=%zu\n", ra, off);
            return 4;
        }
        std::memcpy(deint_a.data(), deint_b_work.data(), 8192);

        std::vector<int8_t> phase90(8192u + 8192u);
        meteor_lrpt::m2x_duplicate_phase90_chunk(chunk, phase90.data(), 8192);
        StreamCtx cb{phase90.data(), phase90.size()};
        const int rb = reader_b.read_samples(stream_pull, &cb, deint_b_work.data(), 8192);
        if (rb != 0) {
            std::fprintf(stderr, "reader_b rc=%d off=%zu\n", rb, off);
            return 5;
        }

        std::array<uint8_t, 1024> frame{};
        if (!pipe.process(deint_a.data(), deint_b_work.data(), false, frame))
            continue;
        if (!post_fec_frame_checks(frame))
            continue;
        int16_t rs_e[4]{-1, -1, -1, -1};
        if (!meteor_lrpt_rs_decode_interleaved_depth4(&frame[4], rs_e))
            continue;
        cadu_stream.insert(cadu_stream.end(), &frame[4], &frame[4] + 1020);
        blocks++;
    }

    if (argc >= 3 && std::strcmp(argv[2], "-") != 0) {
        FILE* o = std::fopen(argv[2], "wb");
        if (!o) {
            std::fprintf(stderr, "open out %s failed\n", argv[2]);
            return 6;
        }
        if (!cadu_stream.empty())
            std::fwrite(cadu_stream.data(), 1, cadu_stream.size(), o);
        std::fclose(o);
    }

    std::printf("m2x_interleaved_ram_pipeline_host ok blocks=%zu cadu_bytes=%zu\n", blocks, cadu_stream.size());
    return 0;
}
