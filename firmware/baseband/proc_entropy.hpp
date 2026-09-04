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

/* Entropy harvester for the "Rand Pwd" external app.
 *
 * This processor does no demodulation. It taps the raw SGPIO/DMA I/Q buffer --
 * the 8-bit samples straight out of the MAX5864, before any decimation -- and
 * harvests bit 0 of I and bit 0 of Q as two independent raw bit streams.
 *
 * The full rationale (why the ADC LSB, why not the MSBs, what the entropy
 * budget is and on what assumption) lives in the header comment of
 * firmware/application/external/random_password/ui_random_password.cpp, which
 * is the other half of this pipeline. Read that first.
 *
 * The M4's jobs, in order:
 *   1. split the buffer into an I-bit and a Q-bit stream;
 *   2. run health tests on the RAW streams (before any conditioning -- that is
 *      the whole point of a health test);
 *   3. estimate per-bit min-entropy with a first-order estimator (float; the
 *      M0 has no FPU, so all the log2 work has to happen here);
 *   4. emit a decimated subset of the raw bits plus an integer milli-bit
 *      credit that the M0 only ever accumulates.
 */

#ifndef __PROC_ENTROPY_HPP__
#define __PROC_ENTROPY_HPP__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "message.hpp"

#include <cstddef>
#include <cstdint>

class EntropyProcessor : public BasebandProcessor {
   public:
    EntropyProcessor();

    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    /* Sample stride within a harvested buffer. Only every Nth sample's LSBs
     * are shipped. The health tests and the estimator still see every sample
     * in the buffer, so the shipped bits are a subset of the tested bits --
     * and being spread further apart in time they are, if anything, less
     * correlated than the population the estimate was made over. */
    static constexpr size_t emit_stride = 8;

    /* Repetition-count cutoff, per channel, in identical consecutive bits.
     * At a fair source P = 2^-127; even at a badly degraded 0.25 bits/bit it
     * is 2^-32. This fires on a stuck, clipped or dead front end and on
     * essentially nothing else. */
    static constexpr uint32_t repetition_cutoff = 128;

    void harvest(const buffer_c8_t& buffer);
    void configure(const EntropyRxConfigureMessage& message);

    bool configured{false};
    bool enabled{false};

    uint32_t buffer_decimation{16};
    uint32_t decimation_phase{0};
    uint32_t settle_remaining{0};

    uint32_t last_cycles{0};
    bool cycle_counter_ok{false};

    EntropyBlockMessage block{};

    size_t baseband_fs{2000000};

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{
        baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};
};

#endif /*__PROC_ENTROPY_HPP__*/
