/*
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

#include "lrpt_deinterleaver.hpp"

#include <cstring>

namespace ui::external_app::lrpt_rx {

using namespace lrpt;

bool Deinterleaver::valid(uint32_t seq) const {
    const uint32_t i = seq % RING_BLOCKS;
    return (valid_[i / 8] >> (i % 8)) & 1;
}

void Deinterleaver::set_valid(uint32_t seq, bool v) {
    const uint32_t i = seq % RING_BLOCKS;
    if (v)
        valid_[i / 8] |= 1 << (i % 8);
    else
        valid_[i / 8] &= ~(1 << (i % 8));
}

uint32_t Deinterleaver::fill_percent() const {
    if (!have_generation_) return 0;
    return (written_ >= DEINT_FILL) ? 100 : written_ * 100 / DEINT_FILL;
}

void Deinterleaver::service(Shared* shared) {
    if (!shared) return;
    const uint32_t generation = shared->generation;
    if (!have_generation_ || generation != generation_) {
        have_generation_ = true;
        generation_ = generation;
        written_ = 0;
        prepared_ = 0;
        memset(valid_, 0, sizeof(valid_));
    }

    // input blocks to the card, oldest first
    for (;;) {
        int slot = -1;
        uint32_t seq = 0;
        for (size_t i = 0; i < DEINT_SLOTS; i++) {
            if (shared->in_state[i] != SLOT_FULL) continue;
            const uint32_t s = shared->in_seq[i];
            if (slot < 0 || s < seq) {
                slot = i;
                seq = s;
            }
        }
        if (slot < 0) break;
        __sync_synchronize();

        // the M4 had no free slot for these
        while (written_ < seq) set_valid(written_++, false);
        const bool ok = write_(context_, (seq % RING_BLOCKS) * FILE_BLOCK, shared->in[slot], DEINT_BLOCK);
        if (!ok) errors_++;
        if (shared->generation != generation) return;
        set_valid(seq, ok);
        if (seq >= written_) written_ = seq + 1;
        shared->in_state[slot] = SLOT_WRITTEN;
    }

    // Output blocks, as soon as a slot is free and their blocks are on the card. Branch k comes
    // from the block (35 - k) * DEINT_LAG before; branch 35 the M4 takes from its input slot.
    for (;;) {
        const uint32_t next = shared->next_consumed;
        if (prepared_ < next) prepared_ = next;  // too late for those
        const size_t slot = prepared_ % DEINT_SLOTS;
        if (shared->out_state[slot] != SLOT_FREE) break;
        if (prepared_ >= DEINT_LAG && prepared_ - DEINT_LAG >= written_) break;

        uint64_t mask = 0;
        for (size_t k = 0; k + 1 < INTER_BRANCHES; k++) {
            const uint32_t lag = (INTER_BRANCHES - 1 - k) * DEINT_LAG;
            if (prepared_ < lag) continue;
            const uint32_t source = prepared_ - lag;
            // not there, or written over since by a block a whole ring later
            if (!valid(source) || written_ > source + RING_BLOCKS) continue;
            if (read_(context_, (source % RING_BLOCKS) * FILE_BLOCK + k * DEINT_SEGMENT,
                      shared->out[slot] + k * DEINT_SEGMENT, DEINT_SEGMENT))
                mask |= (uint64_t)1 << k;
            else
                errors_++;
        }
        if (shared->generation != generation) return;
        // the card was slow and the M4 is past this block: leave the slot free
        if (prepared_ < shared->next_consumed) continue;
        shared->out_seq[slot] = prepared_;
        shared->out_valid[slot] = mask;
        __sync_synchronize();
        shared->out_state[slot] = SLOT_READY;
        // the M4 started over meanwhile
        if (shared->generation != generation) {
            shared->out_state[slot] = SLOT_FREE;
            return;
        }
        prepared_++;
    }
}

}  // namespace ui::external_app::lrpt_rx
