/*
 * Copyright (C) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "meteor_lrpt_deint_service.hpp"

#include "meteor_lrpt_sector_file_ring.hpp"
#include "meteor_lrpt/meteor_deinterleaver.hpp"
#include "meteor_lrpt/meteor_deinterleaver_reader.hpp"
#include "meteor_lrpt/meteor_m2x_interleaved.hpp"
#include "portapack_shared_memory.hpp"

#include "hal.h"

#include "ch.h"

#include <algorithm>
#include <array>
#include <cstring>

namespace meteor_lrpt {
namespace {

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

MeteorDeintFileRing g_ring_a{};
MeteorDeintFileRing g_ring_b{};
/** Shared between A/B readers: `process_one_block` calls them sequentially, never nested. */
std::array<uint8_t, meteor_lrpt::kDeinterleaverReaderHardScratchBytes> g_reader_hard{};
MeteorDeinterleaverReader g_reader_a{g_ring_a, g_reader_hard.data(), &g_ring_a};
MeteorDeinterleaverReader g_reader_b{g_ring_b, g_reader_hard.data(), &g_ring_b};

/** B-branch work buffer: first 8192 bytes are IPC `deint_b`; tail holds `read_samples` scratch (same layout as former `g_reader_scratch`). */
constexpr size_t kM4DeintBWorkBytes =
    meteor_deinterleave_num_samples(8192, 0) + (size_t)(kInterMarkerStride / 2) + 32u;

std::array<int8_t, 8192> g_m4_soft_in{};
std::array<int8_t, 8192> g_m4_deint_a{};
std::array<int8_t, kM4DeintBWorkBytes> g_m4_deint_b{};

Thread* g_thd{nullptr};
uint8_t g_flags{0};
bool g_rings_open{false};

static constexpr eventmask_t EVT_LRPT_DEINT_READY = 1U << 4;

static void open_rings_if_needed() {
    if (g_rings_open)
        return;
    const std::filesystem::path pa{"/LRPT/DEINT_A.BIN"};
    const std::filesystem::path pb{"/LRPT/DEINT_B.BIN"};
    const auto ea = g_ring_a.create(pa);
    const auto eb = g_ring_b.create(pb);
    if (ea.is_valid() || eb.is_valid()) {
        g_ring_a.close();
        g_ring_b.close();
        g_rings_open = false;
        return;
    }
    g_rings_open = true;
}

static void close_rings() {
    g_ring_a.close();
    g_ring_b.close();
    g_rings_open = false;
}

static void process_one_block() {
    auto& ipc = shared_memory.meteor_lrpt_ipc;
    if (ipc.state != 1u || ipc.soft_in == nullptr || ipc.deint_a == nullptr || ipc.deint_b == nullptr)
        return;

    const uint32_t post_seq = ipc.seq;

    int8_t* const soft = g_m4_soft_in.data();
    if (ipc.soft_in != soft)
        return;
    StreamCtx ca{soft, 8192};

    const int ra = g_reader_a.read_samples(stream_pull, &ca, g_m4_deint_b.data(), 8192);
    if (ra != 0) {
        if (ra == 2) {
            const uint32_t n = (uint32_t)ipc.sd_deint_errors + 1u;
            ipc.sd_deint_errors = (uint16_t)std::min<uint32_t>(n, 0xFFFFu);
            __DMB();
        }
        ipc.dropped++;
        ipc.state = 0;
        __DMB();
        return;
    }
    std::memcpy(g_m4_deint_a.data(), g_m4_deint_b.data(), 8192);

    /* B branch: phase90 duplicate then deinterleave into same work buffer (first 8192 B bytes for M4). */
    m2x_duplicate_phase90_chunk(soft, g_m4_deint_b.data(), 8192);
    StreamCtx cb{g_m4_deint_b.data(), 8192};

    const int rb = g_reader_b.read_samples(stream_pull, &cb, g_m4_deint_b.data(), 8192);
    if (rb != 0) {
        if (rb == 2) {
            const uint32_t n = (uint32_t)ipc.sd_deint_errors + 1u;
            ipc.sd_deint_errors = (uint16_t)std::min<uint32_t>(n, 0xFFFFu);
            __DMB();
        }
        ipc.dropped++;
        ipc.state = 0;
        __DMB();
        return;
    }
    __DMB();
    if (ipc.state == 1u && ipc.soft_in == soft && ipc.seq == post_seq) {
        ipc.state = 2u;
        __DMB();
    } else {
        ipc.dropped++;
        ipc.state = 0u;
        __DMB();
    }
}

static WORKING_AREA(wa_deint, 3072);

static msg_t thd_fn(void*) {
    while (true) {
        if (chThdShouldTerminate())
            break;
        (void)chEvtWaitAnyTimeout(EVT_LRPT_DEINT_READY, MS2ST(50));
        if (shared_memory.meteor_lrpt_ipc.magic == SharedMemory::MeteorLrptIpc::kMagic &&
            (g_flags & (1u << 1)) != 0 && shared_memory.meteor_lrpt_ipc.state == 1u)
            process_one_block();
    }
    return 0;
}

}  // namespace

void deint_service_configure(uint8_t meteor_flags) {
    g_flags = meteor_flags;
    auto& ipc = shared_memory.meteor_lrpt_ipc;
    const bool interleaved = (meteor_flags & (1u << 1)) != 0;
    const bool m2x = (meteor_flags & 1u) != 0;

    if (!m2x || !interleaved) {
        ipc.magic = 0;
        ipc.state = 0;
        close_rings();
        return;
    }

    ipc.soft_in = g_m4_soft_in.data();
    ipc.deint_a = g_m4_deint_a.data();
    ipc.deint_b = g_m4_deint_b.data();
    ipc.flags = meteor_flags;
    ipc.state = 0;
    ipc.m4_exec_at_post = 0;
    ipc.seq = 0;
    ipc.sd_deint_errors = 0;
    open_rings_if_needed();
    if (!g_rings_open) {
        ipc.magic = 0;
        __DMB();
        return;
    }
    ipc.magic = SharedMemory::MeteorLrptIpc::kMagic;
    __DMB();
}

void deint_service_start_thread() {
    if (g_thd)
        return;
    g_thd = chThdCreateStatic(wa_deint, sizeof(wa_deint), NORMALPRIO - 4, thd_fn, nullptr);
}

void deint_service_stop_thread() {
    if (!g_thd)
        return;
    chThdTerminate(g_thd);
    chEvtSignal(g_thd, EVT_LRPT_DEINT_READY);
    chThdWait(g_thd);
    g_thd = nullptr;
}

void deint_service_signal_from_isr(void) {
    Thread* const tp = g_thd;
    if (!tp)
        return;
    auto& ipc = shared_memory.meteor_lrpt_ipc;
    if (ipc.magic != SharedMemory::MeteorLrptIpc::kMagic)
        return;
    if (ipc.state != 1u)
        return;
    chEvtSignalI(tp, EVT_LRPT_DEINT_READY);
}

}  // namespace meteor_lrpt
