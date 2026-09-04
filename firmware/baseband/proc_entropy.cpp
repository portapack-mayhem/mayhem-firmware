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

#include "proc_entropy.hpp"

#include "event_m4.hpp"
#include "portapack_shared_memory.hpp"

#include <ch.h>
#include <hal.h>

#include <algorithm>
#include <cmath>

EntropyProcessor::EntropyProcessor() {
    /* Cycle counter, used only as an uncredited jitter source. If the debug
     * block is unavailable the counter stays put, we notice here, and we
     * simply ship zero -- nothing downstream depends on it. */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    const uint32_t a = DWT->CYCCNT;
    const uint32_t b = DWT->CYCCNT;
    cycle_counter_ok = (a != b);
    last_cycles = b;
}

void EntropyProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured || !enabled || buffer.count == 0)
        return;

    if (settle_remaining > 0) {
        settle_remaining--;
        return;
    }

    /* Only one buffer in N is looked at. Entropy rate is not the binding
     * constraint here -- the raw stream carries roughly 4 Mbit/s of candidate
     * bits at 2 Msps, thousands of times what the app can spend -- so we buy
     * M4 headroom with the surplus instead of chasing throughput. */
    if (++decimation_phase < buffer_decimation)
        return;
    decimation_phase = 0;

    harvest(buffer);
}

void EntropyProcessor::harvest(const buffer_c8_t& buffer) {
    const size_t count = buffer.count;

    /* Per channel: 0 = I, 1 = Q. Kept separate all the way through, because a
     * DC-stuck front end produces two individually-constant channels, which is
     * caught instantly by a per-channel repetition test but looks like a
     * healthy 50/50 alternation once you interleave them. */
    uint32_t ones[2] = {0, 0};
    uint32_t transitions[2] = {0, 0};
    uint32_t run_length[2] = {1, 1};
    uint32_t run_max[2] = {1, 1};
    uint8_t prev[2] = {0, 0};

    size_t out_bit = 0;
    std::fill(std::begin(block.data), std::end(block.data), 0);

    for (size_t i = 0; i < count; i++) {
        const uint8_t bit[2] = {
            static_cast<uint8_t>(buffer.p[i].real() & 1),
            static_cast<uint8_t>(buffer.p[i].imag() & 1)};

        for (size_t c = 0; c < 2; c++) {
            ones[c] += bit[c];

            if (i > 0) {
                if (bit[c] != prev[c]) {
                    transitions[c]++;
                    run_length[c] = 1;
                } else {
                    run_length[c]++;
                    if (run_length[c] > run_max[c])
                        run_max[c] = run_length[c];
                }
            }
            prev[c] = bit[c];
        }

        if ((i % emit_stride) == 0 &&
            out_bit + 2 <= EntropyBlockMessage::block_bytes * 8) {
            for (size_t c = 0; c < 2; c++) {
                if (bit[c])
                    block.data[out_bit >> 3] |= (1u << (out_bit & 7));
                out_bit++;
            }
        }
    }

    /* Health tests. All three are deliberately loose: they exist to make a
     * broken source visible, not to police a working one. A false alarm on a
     * healthy source is worse than useless here, because it would train the
     * user to ignore the alarm. */
    uint8_t flags = 0;

    for (size_t c = 0; c < 2; c++) {
        if (run_max[c] >= repetition_cutoff)
            flags |= (c == 0) ? EntropyBlockMessage::health_repetition_i
                              : EntropyBlockMessage::health_repetition_q;

        /* Proportion: fail outside [25%, 75%] ones. On a fair source the
         * probability of reaching that over a 2048-bit window is far below
         * 2^-100, so this only fires on gross bias. */
        if (ones[c] * 4 < count || ones[c] * 4 > count * 3)
            flags |= (c == 0) ? EntropyBlockMessage::health_proportion_i
                              : EntropyBlockMessage::health_proportion_q;

        /* Lag-1 transitions, same cutoff. This is the test the SP 800-90B pair
         * does not give you: neither repetition-count nor adaptive-proportion
         * notices a period-2 pattern, which is exactly what a half-stuck ADC
         * channel looks like. */
        const uint32_t pairs = (count > 1) ? (count - 1) : 1;
        if (transitions[c] * 4 < pairs || transitions[c] * 4 > pairs * 3)
            flags |= (c == 0) ? EntropyBlockMessage::health_transition_i
                              : EntropyBlockMessage::health_transition_q;
    }

    /* First-order min-entropy estimate. This is the ONLY floating point in the
     * pipeline and it lives here because the M0 has no FPU.
     *
     * It is a first-order estimator over one buffer. It cannot see
     * higher-order or long-period structure, and it is therefore used only to
     * pull the credit DOWN below the fixed cap -- never to raise it. */
    const auto h_min_of = [](uint32_t k, uint32_t n) -> float {
        if (n == 0) return 0.0f;
        const uint32_t majority = std::max(k, n - k);
        const float p = static_cast<float>(majority) / static_cast<float>(n);
        if (p >= 1.0f) return 0.0f;
        return -log2f(p);
    };

    float h = 1.0f;
    for (size_t c = 0; c < 2; c++) {
        const float h_bias = h_min_of(ones[c], count);
        const float h_trans = h_min_of(transitions[c], (count > 1) ? count - 1 : 1);
        h = std::min(h, std::min(h_bias, h_trans));
    }
    if (!(h >= 0.0f)) h = 0.0f;  // NaN guard

    /* Credit: min(0.125, 0.5 * h) bits per raw bit shipped.
     *
     * 0.125 is an 8x discount on the physical claim that a dithered ADC LSB is
     * close to a fair coin. The 0.5 factor means full credit still requires a
     * measured 0.25 bits/bit -- a source already four times worse than
     * expected. Below that the credit falls off proportionally and the user
     * watches the progress bar slow down, which is the honest behaviour. */
    const float per_bit = std::min(0.125f, 0.5f * h);
    const uint32_t bits_shipped = static_cast<uint32_t>(out_bit);
    const uint32_t credit_mbits =
        flags ? 0u
              : static_cast<uint32_t>(static_cast<float>(bits_shipped) * per_bit * 1000.0f);

    uint32_t jitter = 0;
    if (cycle_counter_ok) {
        const uint32_t now = DWT->CYCCNT;
        jitter = now - last_cycles;
        last_cycles = now;
    }

    block.jitter = jitter;
    block.credit_mbits = credit_mbits;
    block.h_min_mbits = static_cast<uint16_t>(std::min(65535.0f, h * 1000.0f));
    block.health_flags = flags;
    block.reserved = 0;

    shared_memory.application_queue.push(block);
}

void EntropyProcessor::configure(const EntropyRxConfigureMessage& message) {
    if (message.sampling_rate != 0 && message.sampling_rate != baseband_fs) {
        baseband_fs = message.sampling_rate;
        baseband_thread.set_sampling_rate(baseband_fs);
    }

    buffer_decimation = std::max<uint32_t>(1, message.buffer_decimation);
    settle_remaining = message.settle_buffers;
    decimation_phase = 0;
    enabled = message.enabled;
    configured = true;
}

void EntropyProcessor::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::EntropyRxConfigure:
            configure(*reinterpret_cast<const EntropyRxConfigureMessage*>(message));
            break;

        default:
            break;
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<EntropyProcessor>()};
    event_dispatcher.run();
    return 0;
}
