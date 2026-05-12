/*
 * Host-only smoke: `M2xInterleavedPostDeintPipeline` on zeroed soft (expect no CADU).
 * Full dual-ring + RS path: `test_m2x_interleaved_ram_pipeline_host.cpp` (see `run_reader_long_golden_docker.sh`).
 * Build (from repo root, Linux / WSL / Git Bash):
 *   g++ -std=c++17 -O2 -I firmware/baseband \
 *       tools/meteor_lrpt/test_m2x_pipeline_host.cpp \
 *       firmware/baseband/meteor_lrpt/meteor_m2x_interleaved_pipeline.cpp \
 *       firmware/baseband/meteor_lrpt/meteor_viterbi12.cpp \
 *       firmware/baseband/meteor_lrpt/meteor_cc_decoder.cpp \
 *       firmware/baseband/meteor_lrpt/meteor_cc_encoder.cpp \
 *       firmware/baseband/meteor_lrpt/meteor_soft_correlate.cpp \
 *       firmware/baseband/meteor_lrpt/meteor_nrzm.cpp \
 *       firmware/baseband/meteor_lrpt/meteor_bpsk_ccsds_deframer.cpp \
 *       -o /tmp/m2x_pipeline_smoke && /tmp/m2x_pipeline_smoke
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "meteor_lrpt/meteor_m2x_interleaved_pipeline.hpp"

#include <array>
#include <cstdio>
#include <cstring>

int main() {
    meteor_lrpt::M2xInterleavedPostDeintPipeline pipe{};
    std::array<int8_t, 8192> a{};
    std::array<int8_t, 8192> b{};
    std::array<uint8_t, 1024> frame{};
    const bool ok = pipe.process(a.data(), b.data(), false, frame);
    std::printf("process_zero_soft ok=%d winner=%u stateA=%d stateB=%d\n", ok ? 1 : 0,
                 (unsigned)pipe.last_viterbi_winner(), pipe.viterbi_state_a(), pipe.viterbi_state_b());
    /* All-zero soft should not produce a valid CADU frame; exit 0 only on expected false. */
    return ok ? 1 : 0;
}
