/*
 * Copyright (C) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "meteor_lrpt_deint_service.hpp"

#include "meteor_lrpt_deint_arena.hpp"
#include "meteor_lrpt_sector_file_ring.hpp"
#include "meteor_lrpt/meteor_deinterleaver.hpp"
#include "meteor_lrpt/meteor_deinterleaver_reader.hpp"
#include "meteor_lrpt/meteor_m2x_interleaved.hpp"
#include "portapack_shared_memory.hpp"
#include "lpc43xx_cpp.hpp"

#include "hal.h"

#include "ch.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <new>

using namespace lpc43xx;

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

MeteorDeintArenaStorage* g_arena{nullptr};
MeteorDeinterleaverReader* g_reader_a{nullptr};
MeteorDeinterleaverReader* g_reader_b{nullptr};
bool g_alloc_failed{false};

Thread* g_thd{nullptr};
uint8_t g_flags{0};
bool g_rings_open{false};
bool g_needs_ring_open{false};

static constexpr eventmask_t EVT_LRPT_DEINT_READY = 1U << 4;

MeteorDeintBlockBuffers* arena_bufs() {
    return g_arena ? &g_arena->bufs : nullptr;
}

static bool ensure_deint_buffers() {
    if (g_arena)
        return g_reader_a && g_reader_b;

    /* chHeapAlloc holds the kernel lock; any IRQ using CH_IRQ_PROLOGUE then panics SV#8. */
    creg::m4txevent::disable();
    chSysDisable();
    void* const mem = chHeapAlloc(nullptr, kMeteorDeintArenaBytes);
    chSysEnable();
    if (!mem) {
        g_alloc_failed = true;
        creg::m4txevent::enable();
        return false;
    }

    g_arena = new (mem) MeteorDeintArenaStorage();
    g_alloc_failed = false;

    auto& bufs = g_arena->bufs;
    g_reader_a = new (g_arena->reader_a_blob) MeteorDeinterleaverReader(g_ring_a, bufs.reader_hard, &g_ring_a);
    g_reader_b = new (g_arena->reader_b_blob) MeteorDeinterleaverReader(g_ring_b, bufs.reader_hard, &g_ring_b);
    const bool ok = g_reader_a && g_reader_b;
    creg::m4txevent::enable();
    return ok;
}

static void release_deint_buffers() {
    if (!g_arena)
        return;

    creg::m4txevent::disable();
    chSysDisable();

    if (g_reader_a) {
        g_reader_a->~MeteorDeinterleaverReader();
        g_reader_a = nullptr;
    }
    if (g_reader_b) {
        g_reader_b->~MeteorDeinterleaverReader();
        g_reader_b = nullptr;
    }

    chHeapFree(g_arena);
    g_arena = nullptr;
    chSysEnable();
    creg::m4txevent::enable();
}

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

static void ensure_rings_open_for_ipc() {
    if (g_rings_open || !g_needs_ring_open)
        return;
    open_rings_if_needed();
    g_needs_ring_open = false;
    auto& ipc = shared_memory.meteor_lrpt_ipc;
    if (g_rings_open && g_reader_a && g_reader_b)
        ipc.magic = SharedMemory::MeteorLrptIpc::kMagic;
    else
        ipc.magic = 0;
    __DMB();
}

static bool stage_ingest_soft(uint32_t& post_seq_out) {
    auto* const bufs = arena_bufs();
    if (!bufs || !g_reader_a || !g_reader_b)
        return false;

    auto& ipc = shared_memory.meteor_lrpt_ipc;
    if (ipc.state != 1u || ipc.soft_in == nullptr || ipc.deint_a == nullptr || ipc.deint_b == nullptr)
        return false;

    post_seq_out = ipc.seq;
    if (ipc.soft_in != bufs->soft_in)
        return false;

    return true;
}

static bool stage_deint_branch_a() {
    auto* const bufs = arena_bufs();
    if (!bufs)
        return false;

    StreamCtx ca{bufs->soft_in, kMeteorDeintSoftBlockBytes};
    const int ra = g_reader_a->read_samples(stream_pull, &ca, bufs->deint_a, kMeteorDeintSoftBlockBytes);
    if (ra != 0) {
        auto& ipc = shared_memory.meteor_lrpt_ipc;
        if (ra == 2) {
            const uint32_t n = (uint32_t)ipc.sd_deint_errors + 1u;
            ipc.sd_deint_errors = (uint16_t)std::min<uint32_t>(n, 0xFFFFu);
            __DMB();
        }
        ipc.dropped++;
        ipc.state = 0;
        __DMB();
        return false;
    }
    return true;
}

static bool stage_deint_branch_b() {
    auto* const bufs = arena_bufs();
    if (!bufs)
        return false;

    m2x_duplicate_phase90_chunk(bufs->soft_in, bufs->branch_b_work, kMeteorDeintSoftBlockBytes);
    StreamCtx cb{bufs->branch_b_work, kMeteorDeintSoftBlockBytes};

    const int rb = g_reader_b->read_samples(stream_pull, &cb, bufs->deint_b, kMeteorDeintSoftBlockBytes);
    if (rb != 0) {
        auto& ipc = shared_memory.meteor_lrpt_ipc;
        if (rb == 2) {
            const uint32_t n = (uint32_t)ipc.sd_deint_errors + 1u;
            ipc.sd_deint_errors = (uint16_t)std::min<uint32_t>(n, 0xFFFFu);
            __DMB();
        }
        ipc.dropped++;
        ipc.state = 0;
        __DMB();
        return false;
    }
    return true;
}

static void stage_publish_complete(const uint32_t post_seq, int8_t* const soft) {
    auto& ipc = shared_memory.meteor_lrpt_ipc;
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

static void process_one_block() {
    uint32_t post_seq = 0;
    if (!stage_ingest_soft(post_seq))
        return;

    auto* const bufs = arena_bufs();
    if (!bufs)
        return;

    if (!stage_deint_branch_a())
        return;
    if (!stage_deint_branch_b())
        return;
    stage_publish_complete(post_seq, bufs->soft_in);
}

static WORKING_AREA(wa_deint, 3072);

static msg_t thd_fn(void*) {
    while (true) {
        if (chThdShouldTerminate())
            break;
        (void)chEvtWaitAnyTimeout(EVT_LRPT_DEINT_READY, MS2ST(50));
        ensure_rings_open_for_ipc();
        if (shared_memory.meteor_lrpt_ipc.magic == SharedMemory::MeteorLrptIpc::kMagic &&
            (g_flags & (1u << 1)) != 0 && shared_memory.meteor_lrpt_ipc.state == 1u)
            process_one_block();
    }
    return 0;
}

}  // namespace

void deint_service_configure(uint8_t meteor_flags) {
    g_flags = meteor_flags;
    g_alloc_failed = false;
    auto& ipc = shared_memory.meteor_lrpt_ipc;
    const bool interleaved = (meteor_flags & (1u << 1)) != 0;
    const bool m2x = (meteor_flags & 1u) != 0;

    if (!m2x || !interleaved) {
        ipc.magic = 0;
        ipc.state = 0;
        g_needs_ring_open = false;
        close_rings();
        release_deint_buffers();
        return;
    }

    if (!ensure_deint_buffers()) {
        ipc.magic = 0;
        ipc.state = 0;
        g_needs_ring_open = false;
        __DMB();
        return;
    }

    auto* const bufs = arena_bufs();
    ipc.soft_in = bufs->soft_in;
    ipc.deint_a = bufs->deint_a;
    ipc.deint_b = bufs->deint_b;
    ipc.flags = meteor_flags;
    ipc.state = 0;
    ipc.m4_exec_at_post = 0;
    ipc.seq = 0;
    ipc.sd_deint_errors = 0;
    g_needs_ring_open = true;
    ipc.magic = 0;
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

bool deint_service_last_alloc_failed() {
    return g_alloc_failed;
}

size_t deint_service_arena_bytes() {
    return kMeteorDeintArenaBytes;
}

bool deint_service_heap_budget_ok() {
    size_t heap_free = 0;
    (void)chHeapStatus(nullptr, &heap_free);
    constexpr size_t kHeadroom = 8192u;
    return heap_free >= kMeteorDeintArenaBytes + kHeadroom;
}

}  // namespace meteor_lrpt
