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

#include "proc_spec_an.hpp"

#include "event_m4.hpp"
#include "portapack_shared_memory.hpp"

#include <algorithm>
#include <cstring>

using namespace spec_an;

/* Big buffers live in .bss rather than in the (heap allocated) processor so
 * the linker's memory report accounts for them. */
static complex8_t slot_samples[2][kDmaBufferSamples];
static float fft_sin_q[kMaxFft / 4 + 1];
static float pow_acc[kMaxSliceBins];
static uint8_t waterfall_mem[kWaterfallRows * kPoints];
static dsp::CFloat fft_mem[kMaxFft] __attribute__((aligned(8)));

static inline void compiler_barrier() {
    __asm__ volatile("" ::: "memory");
}

SpecAnProcessor::SpecAnProcessor() {
    dsp::init_sin_table(fft_sin_q);
}

/* Baseband thread ********************************************************/

void SpecAnProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    if (captured_pending) post_captured();

    const uint32_t seq = shared->req_seq;
    if (seq != armed_seq) {
        compiler_barrier();
        armed_seq = seq;
        cap_slice = shared->req_slice;
        cap_sweep = shared->req_sweep;
        settle_left = shared->req_settle;
        blocks_left = avg_blocks;
        capturing = true;
        captured_pending = false; /* superseded */
    }

    if (!capturing) return;

    if (settle_left) {
        settle_left--;
        return;
    }

    Slot& slot = slots[slot_wr];
    if (slot.full) return; /* worker is behind, take the next buffer */

    const size_t blocks = std::min(blocks_left, blocks_per_buffer);
    std::memcpy(slot_samples[slot_wr], buffer.p, (blocks << log2n) * sizeof(complex8_t));
    slot.seq = seq;
    slot.slice = cap_slice;
    slot.sweep = cap_sweep;
    slot.blocks = blocks;
    slot.last = (blocks == blocks_left);
    compiler_barrier();
    slot.full = true;
    slot_wr ^= 1;

    blocks_left -= blocks;
    EventDispatcher::events_flag(EVT_MASK_SPECTRUM);

    if (blocks_left == 0) {
        capturing = false;
        captured_seq = seq;
        captured_pending = true;
        post_captured();
    }
}

void SpecAnProcessor::post_captured() {
    /* push() fails if the event thread holds the queue mutex right now;
     * retried on the next buffer. */
    const SpecAnCapturedMessage message{gen, captured_seq};
    if (shared_memory.application_queue.push(message)) {
        captured_pending = false;
    }
}

/* Event thread ***********************************************************/

void SpecAnProcessor::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::SpecAnConfig:
            configure(*reinterpret_cast<const SpecAnConfigMessage*>(message));
            break;

        case Message::ID::UpdateSpectrum:
            process_pending();
            break;

        default:
            break;
    }
}

void SpecAnProcessor::configure(const SpecAnConfigMessage& message) {
    configured = false;
    compiler_barrier();

    shared = message.shared;
    gen = message.gen;
    log2n = std::clamp<size_t>(message.fft_log2n, kMinFftLog2, kMaxFftLog2);
    n = size_t{1} << log2n;
    blocks_per_buffer = kDmaBufferSamples >> log2n;
    avg_blocks = std::max<size_t>(message.avg_blocks, 1);
    bin_first = message.bin_first;
    bins_per_slice = std::clamp<size_t>(message.bins_per_slice, 1, std::min(n - 16, kMaxSliceBins));
    slices = std::max<size_t>(message.slices, 1);
    window = message.window;

    const float px_per_bin = static_cast<float>(message.px_per_bin_num) / static_cast<float>(message.px_per_bin_den);
    detector.configure(px_per_bin, message.detector, dsp::norm_db(n, window, avg_blocks), slices);

    for (auto& slot : slots) slot.full = false;
    slot_wr = 0;
    slot_rd = 0;
    capturing = false;
    captured_pending = false;
    armed_seq = shared->req_seq;
    acc_valid = false;

    shared->waterfall = waterfall_mem;
    baseband_thread.set_sampling_rate(message.sampling_rate);

    compiler_barrier();
    configured = true;
}

void SpecAnProcessor::process_pending() {
    if (!configured) return;

    /* Bounded so a continuous capture cannot starve the M0's messages. */
    for (size_t iter = 0; iter < slots.size(); iter++) {
        Slot& slot = slots[slot_rd];
        if (!slot.full) break;
        compiler_barrier();

        const uint32_t seq = slot.seq;
        const uint16_t slice = slot.slice;
        const uint8_t sweep = slot.sweep;
        const size_t blocks = slot.blocks;
        const bool last = slot.last;
        const int8_t* const samples = reinterpret_cast<const int8_t*>(slot_samples[slot_rd]);

        if (!acc_valid || seq != acc_seq) {
            std::fill_n(pow_acc, bins_per_slice, 0.0f);
            acc_seq = seq;
            acc_valid = true;
        }

        for (size_t b = 0; b < blocks; b++) {
            dsp::load_block(fft_mem, samples + 2 * (b << log2n), log2n);
            if (b + 1 == blocks) {
                /* Samples are in fft_mem: hand the slot back before the FFT. */
                compiler_barrier();
                slot.full = false;
                slot_rd ^= 1;
            }
            dsp::fft(fft_mem, log2n, fft_sin_q);
            dsp::window_power(fft_mem, n, bin_first, bins_per_slice, window, pow_acc);
        }

        if (last) {
            acc_valid = false;
            finish_slice(slice, sweep);
        }
    }

    if (slots[slot_rd].full) {
        /* More queued: let pending baseband messages through first. */
        EventDispatcher::events_flag(EVT_MASK_SPECTRUM);
    }
}

void SpecAnProcessor::finish_slice(uint16_t slice, uint8_t sweep) {
    detector.run(pow_acc, bins_per_slice, slice, sweep, shared->pix[sweep & 1]);

    const bool last = (slice + 1u) >= slices;
    const SpecAnSliceMessage message{gen, slice, sweep, last, detector.px_begin(), detector.px_end()};

    /* execute() cannot hold the queue mutex while this lower priority thread
     * runs, so a failure here means the queue is full: retry briefly, then
     * drop (those points stay stale for one sweep). */
    for (size_t tries = 0; tries < 50; tries++) {
        if (shared_memory.application_queue.push(message)) break;
        chThdSleepMilliseconds(1);
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<SpecAnProcessor>()};
    event_dispatcher.run();
    return 0;
}
