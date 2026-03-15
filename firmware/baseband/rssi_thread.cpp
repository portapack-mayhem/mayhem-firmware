/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "rssi_thread.hpp"

#include "rssi.hpp"
#include "rssi_dma.hpp"
#include "rssi_stats_collector.hpp"

#include "message.hpp"
#include "portapack_shared_memory.hpp"

#ifdef PRALINE
/*
 * =============================================================================
 * PRALINE Software RSSI
 * =============================================================================
 *
 * Architecture:
 * - baseband_thread: Copies 8 packed I/Q samples (no computation)
 * - rssi_thread: __SMUAD power calc, peak detection, LUT, trackers
 *
 * All DSP happens here to keep baseband_thread minimal.
 */

/*
 * Power-to-RSSI Lookup Table (32 entries)
 *
 * Maps I²+Q² power to RSSI (0-255) using logarithmic scaling.
 * For 8-bit I/Q: max |I|=|Q|=127, so max I²+Q² = 32258
 *
 * Formula: rssi = 32 * log2((index * 8) + 1), clamped to 255
 *
 * With >> 8 scaling (4x more sensitive than >> 10):
 * The trade-off: more sensitivity means the meter saturates (hits max) at lower signal levels.
 * We'll want the bar to be mid-range at typical signal levels, not pegged at max.
 * - Index 0: power 0-255 (I/Q magnitude ~11)
 * - Index 31: power 7936+ (I/Q magnitude ~63+)
 */
static constexpr uint8_t power_to_rssi_lut[32] = {
    0, 101, 130, 148, 161, 171, 179, 186,
    192, 197, 202, 206, 210, 213, 217, 220,
    222, 225, 227, 230, 232, 234, 236, 238,
    240, 241, 243, 244, 246, 247, 249, 255};

/*
 * Convert power to RSSI using 32-entry LUT.
 * Uses >> 8 scaling for 4x more sensitivity than original >> 10.
 * If more sensitivity is needed:
 *     use power >= 4096 & >> 7, yields 8x more sensitivity than >> 10
 *     use power >= 2048 & >> 6, yields 16x more sensitiviy than >> 10
 *     use power >= 1024 & >> 5, yields 32x more sensitiviy than >> 10
 */
static inline uint8_t power_to_rssi(uint32_t power) {
    // uint8_t index = (power >= 32768) ? 31 : static_cast<uint8_t>(power >> 10);
    uint8_t index = (power >= 2048) ? 31 : static_cast<uint8_t>(power >> 6);
    return power_to_rssi_lut[index];
}

/*
 * IIR Smoothing Filter (Exponential Moving Average)
 * Formula: smooth = (current + 7*smooth) / 8  (α = 1/8)
 */
class IIRFilter {
   public:
    uint8_t update(uint8_t current) {
        uint16_t current_q8 = static_cast<uint16_t>(current) << 8;
        smooth_q8_ = (current_q8 + 7 * smooth_q8_) >> 3;
        return static_cast<uint8_t>(smooth_q8_ >> 8);
    }

   private:
    uint16_t smooth_q8_ = 0;
};

/*
 * Running min tracker with decay.
 * Instantly captures new minimums, slowly decays upward.
 */
class MinTracker {
   public:
    uint8_t update(uint8_t current) {
        if (current < min_) {
            min_ = current;
        } else {
            // Slow decay upward (α = 1/16)
            min_ = min_ + ((current - min_) >> 4);
        }
        return min_;
    }

   private:
    uint8_t min_ = 255;
};

/*
 * Running max tracker with decay.
 * Instantly captures new maximums, slowly decays downward.
 */
class MaxTracker {
   public:
    uint8_t update(uint8_t current) {
        if (current > max_) {
            max_ = current;
        } else {
            // Slow decay downward (α = 1/16)
            max_ = max_ - ((max_ - current) >> 4);
        }
        return max_;
    }

   private:
    uint8_t max_ = 0;
};
#endif  // PRALINE

WORKING_AREA(rssi_thread_wa, 128);

Thread* RSSIThread::thread = nullptr;

RSSIThread::RSSIThread(bool auto_start, tprio_t priority)
    : priority_{priority} {
    if (auto_start) start();
}

RSSIThread::~RSSIThread() {
    if (thread) {
        chThdTerminate(thread);
        chThdWait(thread);
        thread = nullptr;
    }
}

void RSSIThread::start() {
    if (!thread) {
        thread = chThdCreateStatic(
            rssi_thread_wa, sizeof(rssi_thread_wa),
            priority_, ThreadBase::fn, this);
    }
}

void RSSIThread::run() {
#ifdef PRALINE

    /*
     * PRALINE (HackRF Pro): Software RSSI from I/Q samples
     *
     * Hardware ADC-based RSSI doesn't work on HackRF Pro.
     *
     * baseband_thread copies 8 packed I/Q samples.
     * We use __SMUAD to compute power for each, find peak,
     * then apply LUT and maintain running stats.
     */

    IIRFilter avg_filter;
    MinTracker min_tracker;
    MaxTracker max_tracker;

    RSSIStatistics stats{};
    uint32_t accumulator = 0;
    uint32_t sample_count = 0;

    constexpr uint32_t samples_per_report = 50;  // ~10Hz reporting at 2ms poll

    while (!chThdShouldTerminate()) {
        chThdSleepMilliseconds(2);  // Poll at ~500Hz

        /*
         * SIMD-accelerated I/Q power calculation for Cortex-M4.
         *
         * Uses __SMUAD (Signed Multiply Accumulate Dual) instruction to compute
         * sum of products of packed halfwords: (a0*a0) + (a1*a1)
         *
         * Processes complex samples per iteration, computing I²+Q² with SIMD.
         * SIMD-accelerated I/Q power calculation for Cortex-M4.
         * ~4x faster than scalar loop.
         * Sum power from all 8 samples (more stable than peak).
         *
         * __SMUAD(val, val) computes:
         *   (low16 * low16) + (high16 * high16) = I² + Q²
         */

        uint32_t total_power = 0;

        for (size_t i = 0; i < 8; i++) {
            const uint32_t packed = shared_memory.software_rssi_iq[i];
            total_power += __SMUAD(packed, packed);
        }

        // Average the 8 samples for min, and avg power
        const uint32_t avg_power = total_power >> 3;

        // Convert to RSSI scale (0-255) via LUT
        const uint8_t avg_rssi = power_to_rssi(avg_power);

        // Update running trackers
        const uint8_t smooth_min = min_tracker.update(avg_rssi);
        const uint8_t smooth_avg = avg_filter.update(avg_rssi);
        const uint8_t smooth_max = max_tracker.update(avg_rssi);

        // Accumulate for periodic report
        accumulator += smooth_avg;
        sample_count++;

        // Report periodically
        if (sample_count >= samples_per_report) {
            stats.min = smooth_min;
            stats.max = smooth_max;
            stats.accumulator = accumulator;
            stats.count = sample_count;

            const RSSIStatisticsMessage message{stats};
            shared_memory.application_queue.push(message);

            // Reset accumulator for next period
            accumulator = 0;
            sample_count = 0;
        }
    }
#else
    /* HackRF One: Use hardware ADC-based RSSI */
    rf::rssi::init();
    rf::rssi::dma::allocate(4, 400);

    RSSIStatisticsCollector stats;

    rf::rssi::start();

    while (!chThdShouldTerminate()) {
        // TODO: Place correct sampling rate into buffer returned here:
        const auto buffer_tmp = rf::rssi::dma::wait_for_buffer();
        const rf::rssi::buffer_t buffer{
            buffer_tmp.p, buffer_tmp.count, sampling_rate};

        stats.process(
            buffer,
            [](const RSSIStatistics& statistics) {
                const RSSIStatisticsMessage message{statistics};
                shared_memory.application_queue.push(message);
            });
    }

    rf::rssi::stop();
    rf::rssi::dma::free();
#endif
}
